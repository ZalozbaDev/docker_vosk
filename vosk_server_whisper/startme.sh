#!/bin/bash

nvidia-smi

/vosk_whisper_server 0.0.0.0 2700 1 ${MODEL_PATH_FULL}

while /bin/true; do sleep 1 ; done

# export VOSK_SAMPLE_RATE=48000 
# gdb /vosk_whisper_server
# set args 0.0.0.0 2700 1 /whisper/Korla/hsb_stt_demo/hsb_whisper/ggml-model.q8_0.bin
# set print thread-events off
# run
