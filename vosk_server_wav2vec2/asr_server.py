import os
import sys
import asyncio
import websockets
import concurrent.futures
import logging
import json
import torch
import numpy as np
import soundfile as sf
from pyctcdecode import build_ctcdecoder
from transformers import pipeline, AutoProcessor, Wav2Vec2ProcessorWithLM
from silero_vad import load_silero_vad
import time


model = load_silero_vad()

def process_chunk(asr_pipeline, sample_rate, message, buffer, silence_dur, speech_dur):
    # is there speech in message?
    if isinstance(message, str) and message == '{"eof" : 1}':
        if not buffer:
            return json.dumps({"text": ""}), True
        audio = np.concatenate(buffer).astype(np.float32) / 32768.0 
        sf.write('temp.wav', audio, int(sample_rate), format='WAV', subtype='PCM_16')
        transcription = asr_pipeline(audio)
        text = transcription["text"] if isinstance(transcription, dict) else transcription[0]["text"]
        return json.dumps({"text": text}, ensure_ascii=False), True
    else:
        audio = np.frombuffer(message, dtype=np.int16)
        buffer.append(audio)
        # Wait for silence to determine if the user has finished speaking
        model_out = model.audio_forward(torch.Tensor(audio), sr=int(sample_rate))
        if model_out.mean().item() < 0.5:
            silence_dur["value"] += len(audio) / sample_rate
        else:
            silence_dur["value"] = 0
            speech_dur["value"] += len(audio) / sample_rate
        audio = np.concatenate(buffer).astype(np.float32) / 32768.0
        model_out = model.audio_forward(torch.Tensor(audio), sr=int(sample_rate))
        if silence_dur["value"] > 0.75 and speech_dur["value"] > 0.25:
            buffer.clear()
            t1 = time.time()
            transcription = asr_pipeline(audio)["text"]
            t2 = time.time()
            print(f"Transcription took {t2 - t1:.2f} seconds. Real-time factor: {(len(audio) / sample_rate) /(t2 - t1) :.2f}x")
            silence_dur["value"] = 0
            speech_dur["value"] = 0
            return json.dumps({"text": transcription}, ensure_ascii=False), False
        elif silence_dur["value"] > 0.75:
            silence_dur["value"] = 0
        return json.dumps({"partial": ""}), False

async def recognize(websocket, path="/"):
    global asr_pipeline
    global args
    global pool

    loop = asyncio.get_running_loop()
    sample_rate = args.sample_rate

    logging.info('Connection from %s', websocket.remote_address)
    buffer = []
    silence_dur = {"value": 0}
    speech_dur = {"value": 0}

    while True:
        message = await websocket.recv()

        # Load config if provided
        if isinstance(message, str) and 'config' in message:
            jobj = json.loads(message)['config']
            logging.info("Config %s", jobj)
            if 'sample_rate' in jobj:
                sample_rate = float(jobj['sample_rate'])
            continue

        response, stop = await loop.run_in_executor(
            pool, process_chunk, asr_pipeline, sample_rate, message, buffer, silence_dur, speech_dur
        )
        await websocket.send(response)
        if stop:
            break

async def start():
    global asr_pipeline
    global args
    global pool

    logging.basicConfig(level=logging.INFO)

    args = type('', (), {})()
    args.interface = os.environ.get('ASR_SERVER_INTERFACE', '0.0.0.0')
    args.port = int(os.environ.get('ASR_SERVER_PORT', 2700))
    args.model_name = os.environ.get('ASR_MODEL_NAME', 'Korla/Wav2Vec2BertForCTC-hsb')
    args.sample_rate = float(os.environ.get('ASR_SAMPLE_RATE', 16000))

    if len(sys.argv) > 1:
        args.model_name = sys.argv[1]

    processor = AutoProcessor.from_pretrained("Korla/Wav2Vec2BertForCTC-hsb")
    processor.feature_extractor._processor_class = "Wav2Vec2ProcessorWithLM"
    vocab_dict = processor.tokenizer.get_vocab()
    sorted_vocab_dict = {k.lower(): v for k, v in sorted(vocab_dict.items(), key=lambda item: item[1])}
    USE_LM = True
    if USE_LM:
        decoder = build_ctcdecoder(
            labels=list(sorted_vocab_dict.keys()),
            kenlm_model_path="./5gram_correct.arpa",
        )
        processor_with_lm = Wav2Vec2ProcessorWithLM(
            feature_extractor=processor.feature_extractor,
            tokenizer=processor.tokenizer,
            decoder=decoder
        )
        asr_pipeline = pipeline("automatic-speech-recognition", model=args.model_name, tokenizer=processor_with_lm, feature_extractor=processor_with_lm.feature_extractor, decoder=processor_with_lm.decoder)
    else:
        asr_pipeline = pipeline("automatic-speech-recognition", model=args.model_name, tokenizer=processor.tokenizer, feature_extractor=processor.feature_extractor)
    pool = concurrent.futures.ThreadPoolExecutor((os.cpu_count() or 1))

    async with websockets.serve(recognize, args.interface, args.port):
        await asyncio.Future()

if __name__ == '__main__':
    asyncio.run(start())