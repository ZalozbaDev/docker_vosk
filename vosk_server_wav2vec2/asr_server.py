import os
import sys
import asyncio
import websockets
import concurrent.futures
import logging
import json
from pathlib import Path
import torch
import numpy as np
import librosa
import gc
from pyctcdecode import build_ctcdecoder
from transformers import AutoProcessor, Wav2Vec2ProcessorWithLM, AutoModelForCTC
from silero_vad import load_silero_vad
import time


model = load_silero_vad()
model.eval()


def load_server_config(config_path):
    defaults = {
        "interface": "0.0.0.0",
        "port": 2700,
        "model_name": "./ov_int8",
        "processor_name": "Korla/Wav2Vec2BertForCTC-hsb-0",
        "sample_rate": 16000,
        "verbose_output": False,
        "backend": "openvino",
        "openvino_device": "CPU",
        "use_lm": False,
        "empty_text_conf_zero": True,
    }
    config = dict(defaults)
    with open(config_path, "r", encoding="utf-8") as f:
        user_cfg = json.load(f)
    if not isinstance(user_cfg, dict):
        raise ValueError("Config root must be a JSON object")
    config.update(user_cfg)
    config["backend"] = str(config["backend"]).lower()
    if config["backend"] not in {"pytorch", "onnx", "openvino"}:
        raise ValueError("backend must be one of: pytorch, onnx, openvino")
    return config


def finalize_confidence(transcription, confidence):
    if args.empty_text_conf_zero and not str(transcription).strip():
        return 0.0
    return float(confidence)


def masked_mean_or_zero(scores, mask):
    valid_scores = scores[mask]
    if valid_scores.numel() == 0:
        return 0.0
    return valid_scores.mean().item()

def cleanup_memory():
    """Clean up memory by freeing GPU memory if available and collecting garbage."""
    try:
        # Force garbage collection to clean up ONNX CPU memory and numpy arrays
        gc.collect()
        
        # Clear GPU cache if CUDA is available
        if torch.cuda.is_available():
            torch.cuda.empty_cache()
    except Exception as e:
        logging.debug(f"Error during memory cleanup: {e}")

def process_chunk(asr_pipeline, sample_rate, message, buffer, silence_dur, speech_dur):
    # is there speech in message?
    if isinstance(message, str) and message == '{"eof" : 1}':
        return json.dumps({"text": "",}, ensure_ascii=False), True
    else:
        audio = np.frombuffer(message, dtype=np.int16)
        # resample if needed
        if sample_rate != 16000:
            audio = librosa.resample(audio.astype(np.float32), orig_sr=sample_rate, target_sr=16000)
            sample_rate = 16000
        buffer.append(audio)
        # Wait for silence to determine if the user has finished speaking
        with torch.inference_mode():
            model_out = model.audio_forward(torch.from_numpy(audio), sr=int(sample_rate))
        if model_out.mean().item() < 0.5:
            silence_dur["value"] += len(audio) / sample_rate
            listening = False
        else:
            silence_dur["value"] = 0
            speech_dur["value"] += len(audio) / sample_rate
            listening = True
        audio = np.concatenate(buffer).astype(np.float32) / 32768.0
        if (silence_dur["value"] > 0.75 and speech_dur["value"] > 0.25) or (len(audio) > sample_rate * 30):
            if len(audio) > sample_rate * 30:
                logging.info("Audio too long, processing what we have so far.")
            buffer.clear()
            t1 = time.time()
            blank_id = asr_pipeline["processor"].tokenizer.pad_token_id
            word_delemiter_id = asr_pipeline["processor"].tokenizer.word_delimiter_token_id
            if args.backend == "onnx":
                logging.info('ONNX decoding')
                inputs = asr_pipeline["processor"](audio, sampling_rate=sample_rate, return_tensors="pt").to(torch.float16)
                onnx_inputs = inputs["input_features"].numpy()
                onnxruntime_outputs = asr_pipeline["model"].run(None, {"input": onnx_inputs})
                logits = torch.from_numpy(onnxruntime_outputs[0])
                predicted_ids = torch.argmax(logits, dim=-1)
                pred_scores = logits.softmax(dim=-1).gather(-1, predicted_ids.unsqueeze(-1))[:, :, 0]
                # Clear large intermediate arrays
                onnx_inputs = None
                onnxruntime_outputs = None
                cleanup_memory()
            elif args.backend == "openvino":
                logging.info('OpenVINO decoding')
                inputs = asr_pipeline["processor"](
                    audio,
                    sampling_rate=sample_rate,
                    return_tensors="np",
                    padding=False,
                )["input_features"].astype(np.float16)
                ov_outputs = asr_pipeline["model"]({asr_pipeline["ov_input_name"]: inputs})
                logits = torch.from_numpy(ov_outputs[asr_pipeline["ov_output"]])
                predicted_ids = torch.argmax(logits, dim=-1)
                pred_scores = logits.softmax(dim=-1).gather(-1, predicted_ids.unsqueeze(-1))[:, :, 0]
                cleanup_memory()
            else:
                logging.info('decoding without ONNX or OpenVINO')
                inputs = asr_pipeline["processor"](audio, sampling_rate=sample_rate, return_tensors="pt", padding=False).to(asr_pipeline["device"], dtype=asr_pipeline["dtype"])
                with torch.no_grad():
                    logits = asr_pipeline["model"](**inputs).logits
                    predicted_ids = torch.argmax(logits, dim=-1)
                    pred_scores = logits.softmax(dim=-1).gather(-1, predicted_ids.unsqueeze(-1))[:, :, 0]
                cleanup_memory()            
            if "decoder" in asr_pipeline:
                logging.info('use LM to rescore results')
                transcription = asr_pipeline["decoder"].decode(predicted_ids[0].cpu().numpy())
                confidence = masked_mean_or_zero(pred_scores, (predicted_ids != blank_id) & (predicted_ids != word_delemiter_id))
                confidence = finalize_confidence(transcription, confidence)
                t2 = time.time()
                print(f"Transcription took {t2 - t1:.2f} seconds. Real-time factor: {(len(audio) / sample_rate) /(t2 - t1) :.2f}x")
                silence_dur["value"] = 0
                speech_dur["value"] = 0
                cleanup_memory()
                return json.dumps({"text": transcription, "conf": confidence}, ensure_ascii=False), False
            elif args.verbose_output:
                logging.info('generate verbose JSON response')
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
                weight_sum = sum(n for _, _, n, _, _, _ in word_confs)
                if weight_sum == 0:
                    weighted_word_mean = 0.0
                else:
                    weighted_word_mean = sum(conf * n for _, conf, n, _, _, _ in word_confs) / weight_sum
                weighted_word_mean = finalize_confidence(transcription, weighted_word_mean)
                t2 = time.time()
                print(f"Transcription took {t2 - t1:.2f} seconds. Real-time factor: {(len(audio) / sample_rate) /(t2 - t1) :.2f}x")
                silence_dur["value"] = 0
                speech_dur["value"] = 0
                cleanup_memory()
                logging.info(json.dumps({"text": transcription, "conf": weighted_word_mean, "result": results}, ensure_ascii=False))
                return json.dumps({"text": transcription, "conf": weighted_word_mean, "result": results}, ensure_ascii=False), False
            else:
                logging.info('normal decoding without LM without verbose result')
                transcription = asr_pipeline["processor"].batch_decode(predicted_ids)[0]
                confidence = masked_mean_or_zero(pred_scores, (predicted_ids != blank_id) & (predicted_ids != word_delemiter_id))
                confidence = finalize_confidence(transcription, confidence)
                t2 = time.time()
                print(f"Transcription took {t2 - t1:.2f} seconds. Real-time factor: {(len(audio) / sample_rate) /(t2 - t1) :.2f}x")
                silence_dur["value"] = 0
                speech_dur["value"] = 0
                cleanup_memory()
                return json.dumps({"text": transcription, "conf": confidence}, ensure_ascii=False), False
        elif silence_dur["value"] > 0.75:
            silence_dur["value"] = 0
            # Clear buffer to prevent unbounded growth on silence-only paths
            if buffer:
                buffer.clear()
                speech_dur["value"] = 0
        cleanup_memory()
        return json.dumps({"partial": "", "listening": listening}), False

async def recognize(websocket, path="/"):
    global asr_pipeline
    global args
    global pool
    global inference_semaphore

    loop = asyncio.get_running_loop()
    sample_rate = args.sample_rate

    logging.info('Connection from %s', websocket.remote_address)
    buffer = []
    silence_dur = {"value": 0}
    speech_dur = {"value": 0}

    try:
        while True:
            message = await websocket.recv()

            # Load config if provided
            if isinstance(message, str) and 'config' in message:
                jobj = json.loads(message)['config']
                logging.info("Config %s", jobj)
                if 'sample_rate' in jobj:
                    sample_rate = float(jobj['sample_rate'])
                continue

            async with inference_semaphore:
                response, stop = await loop.run_in_executor(
                    pool, process_chunk, asr_pipeline, sample_rate, message, buffer, silence_dur, speech_dur
                )
            await websocket.send(response)
            if stop:
                break
    finally:
        # Clean up buffer and state when connection closes
        buffer.clear()
        silence_dur["value"] = 0
        speech_dur["value"] = 0
        logging.info('Connection from %s closed', websocket.remote_address)
        cleanup_memory()

async def start():
    global asr_pipeline
    global args
    global pool

    logging.basicConfig(level=logging.INFO)

    args = type('', (), {})()
    config_path = os.environ.get('ASR_CONFIG_PATH', './asr_server_config.json')
    if len(sys.argv) > 1:
        if sys.argv[1] in {'-c', '--config'} and len(sys.argv) > 2:
            config_path = sys.argv[2]
        else:
            config_path = sys.argv[1]
    cfg = load_server_config(config_path)

    args.interface = cfg["interface"]
    args.port = int(cfg["port"])
    args.model_name = cfg["model_name"]
    args.processor_name = cfg.get("processor_name", cfg["model_name"])
    args.sample_rate = float(cfg["sample_rate"])
    args.verbose_output = bool(cfg["verbose_output"])
    args.backend = cfg["backend"]
    args.onnx = args.backend == "onnx"
    args.openvino = args.backend == "openvino"
    args.use_lm = bool(cfg["use_lm"])
    args.openvino_device = cfg.get("openvino_device", "CPU")
    args.empty_text_conf_zero = bool(cfg.get("empty_text_conf_zero", True))

    logging.info('Loaded ASR config from %s', config_path)
    logging.info('ASR backend: %s', args.backend)

    if args.verbose_output:
        logging.info('Verbose JSON return enabled.')
        import hunspell
        global hobj
        hobj = hunspell.HunSpell("./spell/hsb.dic", "./spell/hsb.aff")


    try:
        processor = AutoProcessor.from_pretrained(args.processor_name)
    except (OSError, ValueError):
        # when processor is not found in the specified path, try to load from Hugging Face Hub
        processor = AutoProcessor.from_pretrained("Korla/Wav2Vec2BertForCTC-hsb-0")
    processor.feature_extractor._processor_class = "Wav2Vec2ProcessorWithLM"
    vocab_dict = processor.tokenizer.get_vocab()
    sorted_vocab_dict = {k.lower(): v for k, v in sorted(vocab_dict.items(), key=lambda item: item[1])}

    if args.onnx:
        import onnxruntime
        model = onnxruntime.InferenceSession(args.model_name, providers=["CPUExecutionProvider"])
        device = "cpu"
        dtype = np.float16
        logging.info('ONNX decoding enabled.')
    elif args.openvino:
        from openvino import Core

        ov_core = Core()
        model_path = Path(args.model_name)
        if model_path.is_dir():
            model_path = model_path / "model.xml"
        ov_model = ov_core.read_model(str(model_path))
        model = ov_core.compile_model(ov_model, args.openvino_device)
        ov_input_name = model.input(0).any_name
        ov_output = model.output(0)
        device = "cpu"
        dtype = np.float32
        logging.info('OpenVINO decoding enabled on device: %s', args.openvino_device)
    else:
        device = "mps" if torch.backends.mps.is_available() else "cuda" if torch.cuda.is_available() else "cpu"
        model = AutoModelForCTC.from_pretrained(args.model_name).to(device)
        dtype = model.dtype
        logging.info('ONNX decoding is NOT enabled.')
        torch.set_num_threads(os.cpu_count() or 1)
        logging.info(f'Using {torch.get_num_threads()} CPU threads for PyTorch.')
    if args.use_lm:
        decoder = build_ctcdecoder(
            labels=list(sorted_vocab_dict.keys()),
            kenlm_model_path="./lm/5gram_correct.arpa",
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
        if args.openvino:
            asr_pipeline["ov_input_name"] = ov_input_name
            asr_pipeline["ov_output"] = ov_output
        logging.info('Use provided ARPA LM.')
    else:
        asr_pipeline = {
            "model": model,
            "processor": processor,
            "device": device,
            "dtype": dtype
        }
        if args.openvino:
            asr_pipeline["ov_input_name"] = ov_input_name
            asr_pipeline["ov_output"] = ov_output
        logging.info('IGNORING any ARPA LM.')
    pool = concurrent.futures.ThreadPoolExecutor(max_workers=1)
    global inference_semaphore
    inference_semaphore = asyncio.Semaphore(1)
    logging.info('Inference serialized: max 1 concurrent decode.')

    async with websockets.serve(recognize, args.interface, args.port):
        await asyncio.Future()

if __name__ == '__main__':
    asyncio.run(start())