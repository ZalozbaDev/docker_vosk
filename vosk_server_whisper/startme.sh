#!/bin/bash

nvidia-smi

LD_LIBRARY_PATH=/:/onnxruntime-linux-x64-1.12.1/lib/ /vosk_whisper_server 0.0.0.0 2700 1 ${MODEL_PATH_FULL}

# while /bin/true; do sleep 1 ; done

# export VOSK_SAMPLE_RATE=48000 
# export LD_LIBRARY_PATH=/:/onnxruntime-linux-x64-1.12.1/lib/
# gdb /vosk_whisper_server
# set args 0.0.0.0 2700 1 /whisper/Korla/whisper_large_v3_turbo_hsb-0/ggml-model.bin
# set print thread-events off
# run
