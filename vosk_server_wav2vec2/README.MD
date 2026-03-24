# Vosk Wav2Vec2 WebSocket Server

Streaming ASR server for Upper Sorbian using Hugging Face Wav2Vec2/CTC models.

This directory contains a Python WebSocket server that accepts raw PCM audio and returns recognized text. It uses:

- `transformers` for model inference
- `silero-vad` for end-of-utterance detection
- optional CTC language model decoding (`pyctcdecode` + KenLM)

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

## Run Locally

From this directory:

```bash
export ASR_SERVER_INTERFACE=0.0.0.0
export ASR_SERVER_PORT=2700
export ASR_MODEL_NAME=Korla/Wav2Vec2BertForCTC-hsb-0
export ASR_SAMPLE_RATE=16000
export ASR_VERBOSE_OUTPUT=true

python asr_server.py
```

You can also pass model name as the first positional CLI arg:

```bash
python asr_server.py Korla/Wav2Vec2BertForCTC-hsb-0
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
