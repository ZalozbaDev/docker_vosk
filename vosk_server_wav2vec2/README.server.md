# Vosk Wav2Vec2 WebSocket Server

Streaming ASR server for Upper Sorbian using Hugging Face Wav2Vec2/CTC models.

This directory contains a Python WebSocket server that accepts raw PCM audio and returns recognized text. It uses:

- `transformers` for model inference
- `silero-vad` for end-of-utterance detection
- optional CTC language model decoding (`pyctcdecode` + KenLM)
- backend selection via JSON config (`pytorch`, `onnx`, `openvino`)

The server is compatible with Vosk-like client behavior.

## Audio Protocol

The server expects mono, signed 16-bit PCM chunks (`int16`) over WebSocket.

### Client -> Server messages

- JSON config (optional at start):

```json
{ "config" : { "sample_rate" : 16000 } }
```

- Binary audio chunks: raw PCM16 bytes
- End-of-stream marker:

```json
{"eof" : 1}
```

### Server -> Client messages

- Partial response while collecting speech:

```json
{"partial": ""}
```


- Final response with confidence (`asr_server2.py`):

```json
{"text": "...", "conf": 0.87}
```

- Verbose word-level output (`ASR_VERBOSE_OUTPUT=true`):

```json
{
	"text": "...",
	"conf": 0.87,
	"results": [
		{"word": "...", "start": 0.12, "end": 0.44, "conf": 0.81}
	]
}
```

## Server Config (JSON)

Startup config is loaded from a JSON file (default: `./asr_server_config.json`).

Example:

```json
{
	"interface": "0.0.0.0",
	"port": 2700,
	"backend": "openvino",
	"model_name": "./ov_int8",
	"processor_name": "Korla/Wav2Vec2BertForCTC-hsb-0",
	"openvino_device": "CPU",
	"sample_rate": 16000,
	"verbose_output": true,
	"use_lm": false
}
```

Notes:

- `backend` supports: `pytorch`, `onnx`, `openvino`.
- For OpenVINO, `model_name` can be a folder (the server will load `openvino_model.xml` from it).
- For ONNX, `model_name` must point to the ONNX model file.

## Run Locally

From this directory:

```bash
python asr_server.py --config ./asr_server_config.json
```

Adjust fields in the JSON config as needed. Default config is set to OpenVINO and loads model files from `./ov_int8`.

Make sure that you have the `hsb.dic` and `hsb.aff` files in the same directory as `asr_server.py` if you have `ASR_VERBOSE_OUTPUT=true` to enable spelling checks.

You can also pass the config path as the first positional CLI arg:

```bash
python asr_server.py ./asr_server_config.json
```

## Test With Microphone

In another terminal:

```bash
python test_mic.py -u ws://127.0.0.1:2700 -r 16000
```

List devices if needed:

```bash
python test_mic.py --list-devices
```
