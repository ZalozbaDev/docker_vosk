#!/bin/bash

LD_LIBRARY_PATH=/data/ /recikts_server 0.0.0.0 2700 1 ${MODEL_PATH_FULL}

# export LD_LIBRARY_PATH=/data/
# export VOSK_SAMPLE_RATE=48000
# gdb /recikts_server
# set args 0.0.0.0 2700 1 /merged.cfg
# set print thread-events off
# run
