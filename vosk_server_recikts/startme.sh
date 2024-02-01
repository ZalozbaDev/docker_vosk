#!/bin/bash

# LD_LIBRARY_PATH=/ VOSK_SAMPLE_RATE=48000 /recikts_server 0.0.0.0 2700 1 /hsb_tri1.cfg
LD_LIBRARY_PATH=/ VOSK_SAMPLE_RATE=48000 /recikts_server 0.0.0.0 2700 1 /merged.cfg

# export LD_LIBRARY_PATH=/
# export VOSK_SAMPLE_RATE=48000
# gdb /recikts_server
# set args 0.0.0.0 2700 1 /merged.cfg
# set print thread-events off
# run
