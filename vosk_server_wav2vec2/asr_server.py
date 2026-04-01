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
from transformers import AutoProcessor, Wav2Vec2ProcessorWithLM, pipeline, AutoModelForCTC
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
            blank_id = asr_pipeline["processor"].tokenizer.pad_token_id
            word_delemiter_id = asr_pipeline["processor"].tokenizer.word_delimiter_token_id
            if args.onnx:
                inputs = asr_pipeline["processor"](audio, sampling_rate=sample_rate, return_tensors="pt").to(torch.float16)
                onnx_inputs = inputs["input_features"].numpy()
                onnxruntime_outputs = asr_pipeline["model"].run(None, {"input": onnx_inputs})
                logits = torch.from_numpy(onnxruntime_outputs[0])
                predicted_ids = torch.argmax(logits, dim=-1)
                pred_scores = logits.softmax(dim=-1).gather(-1, predicted_ids.unsqueeze(-1))[:, :, 0]
            else:
                inputs = asr_pipeline["processor"](audio, sampling_rate=sample_rate, return_tensors="pt", padding=False).to(asr_pipeline["device"], dtype=asr_pipeline["dtype"])
                with torch.no_grad():
                    logits = asr_pipeline["model"](**inputs).logits
                    predicted_ids = torch.argmax(logits, dim=-1)
                    pred_scores = logits.softmax(dim=-1).gather(-1, predicted_ids.unsqueeze(-1))[:, :, 0]
            if "decoder" in asr_pipeline:
                transcription = asr_pipeline["decoder"].decode(predicted_ids[0].cpu().numpy())
                confidence = pred_scores[(predicted_ids != blank_id) & (predicted_ids != word_delemiter_id)].mean().item()
                t2 = time.time()
                print(f"Transcription took {t2 - t1:.2f} seconds. Real-time factor: {(len(audio) / sample_rate) /(t2 - t1) :.2f}x")
                silence_dur["value"] = 0
                speech_dur["value"] = 0
                return json.dumps({"text": transcription, "conf": confidence}, ensure_ascii=False), False
            elif args.verbose_output:
                transcription = asr_pipeline["processor"].batch_decode(predicted_ids)[0]
                splitted_ids = []
                predicted_ids = predicted_ids[0]
                for idx in predicted_ids:
                    if idx == word_delemiter_id and splitted_ids[-1]:
                        splitted_ids.append([])
                    else:
                        if not splitted_ids:
                            splitted_ids.append([])
                        splitted_ids[-1].append(idx.item())
                if predicted_ids.dim() == 2:
                    ids = predicted_ids[0]
                else:
                    ids = predicted_ids

                if pred_scores.dim() == 2:
                    scores = pred_scores[0]
                else:
                    scores = pred_scores
                valid_mask = (ids != blank_id) & (ids != word_delemiter_id)
                sentence_conf = scores[valid_mask].mean().item()
                print("sentence_conf:", sentence_conf)
                frame_shift_s = 0.04
                words = []
                current_ids = []
                current_scores = []
                current_start = None
                current_end = None
                for frame_idx, (token_id, token_score) in enumerate(zip(ids.tolist(), scores.tolist())):
                    if token_id == word_delemiter_id:
                        if current_ids:
                            words.append((current_ids, current_scores, current_start, current_end))
                            current_ids = []
                            current_scores = []
                            current_start = None
                            current_end = None
                        continue
                    if token_id == blank_id:
                        continue
                    if current_start is None:
                        current_start = frame_idx
                    current_end = frame_idx + 1
                    current_ids.append(token_id)
                    current_scores.append(token_score)
                if current_ids:
                    words.append((current_ids, current_scores, current_start, current_end))
                word_confs = []
                results = []
                for word_ids, word_scores, start_frame, end_frame in words:
                    word_conf = float(sum(word_scores) / len(word_scores))
                    word_text = asr_pipeline["processor"].decode(word_ids).strip()
                    start_s = start_frame * frame_shift_s
                    end_s = end_frame * frame_shift_s
                    spelling = hobj.spell(word_text)
                    word_confs.append((word_text, word_conf, len(word_scores), start_s, end_s, spelling))
                    results.append({"conf": word_conf, "end": end_s, "start": start_s, "word": word_text, "spelling": spelling})
                weighted_word_mean = (sum(conf * n for _, conf, n, _, _, _ in word_confs)/ sum(n for _, _, n, _, _, _ in word_confs))
                t2 = time.time()
                print(f"Transcription took {t2 - t1:.2f} seconds. Real-time factor: {(len(audio) / sample_rate) /(t2 - t1) :.2f}x")
                silence_dur["value"] = 0
                speech_dur["value"] = 0
                return json.dumps({"text": transcription, "conf": weighted_word_mean, "results": results}, ensure_ascii=False), False
            else:
                transcription = asr_pipeline["processor"].batch_decode(predicted_ids)[0]
                confidence = pred_scores[(predicted_ids != blank_id) & (predicted_ids != word_delemiter_id)].mean().item()
                t2 = time.time()
                print(f"Transcription took {t2 - t1:.2f} seconds. Real-time factor: {(len(audio) / sample_rate) /(t2 - t1) :.2f}x")
                silence_dur["value"] = 0
                speech_dur["value"] = 0
                return json.dumps({"text": transcription, "conf": confidence}, ensure_ascii=False), False
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
    args.model_name = os.environ.get('ASR_MODEL_NAME', 'Korla/Wav2Vec2BertForCTC-hsb-0')
    args.sample_rate = float(os.environ.get('ASR_SAMPLE_RATE', 16000))
    args.verbose_output = os.environ.get('ASR_VERBOSE_OUTPUT', 'false').lower() == 'true'
    args.onnx = os.environ.get('ASR_ONNX', 'false').lower() == 'true'

    if args.verbose_output:
        import hunspell
        global hobj
        hobj = hunspell.HunSpell("hsb.dic", "hsb.aff")


    if len(sys.argv) > 1:
        args.model_name = sys.argv[1]

    processor = AutoProcessor.from_pretrained("Korla/Wav2Vec2BertForCTC-hsb-0")
    processor.feature_extractor._processor_class = "Wav2Vec2ProcessorWithLM"
    vocab_dict = processor.tokenizer.get_vocab()
    sorted_vocab_dict = {k.lower(): v for k, v in sorted(vocab_dict.items(), key=lambda item: item[1])}
    USE_LM = False
    if args.onnx:
        import onnxruntime
        model = onnxruntime.InferenceSession("./onnx/wav2vec2.onnx", providers=["CPUExecutionProvider"])
        device = "cpu"
        dtype = np.float16
    else:
        device = "mps" if torch.backends.mps.is_available() else "cuda" if torch.cuda.is_available() else "cpu"
        model = AutoModelForCTC.from_pretrained(args.model_name).to(device)
        dtype = model.dtype
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
        asr_pipeline = {
            "model": model,
            "processor": processor_with_lm,
            "decoder": decoder,
            "device": device,
            "dtype": dtype
        }
    else:
        asr_pipeline = {
            "model": model,
            "processor": processor,
            "device": device,
            "dtype": dtype
        }
    pool = concurrent.futures.ThreadPoolExecutor((os.cpu_count() or 1))

    async with websockets.serve(recognize, args.interface, args.port):
        await asyncio.Future()

if __name__ == '__main__':
    asyncio.run(start())