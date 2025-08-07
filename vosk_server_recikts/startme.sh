#!/bin/bash

LD_LIBRARY_PATH=/data/:/onnxruntime-linux-x64-1.12.1/lib/ /recikts_server 0.0.0.0 2700 1 ${MODEL_PATH_FULL}

# while :; do sleep 1; done

if /bin/false; then
	export LD_LIBRARY_PATH=/data/:/onnxruntime-linux-x64-1.12.1/lib/
	export VOSK_SAMPLE_RATE=48000
	gdb /recikts_server
	set args 0.0.0.0 2700 1 /data/misa_2025_03_05.cfg
	set print thread-events off
	run
fi
