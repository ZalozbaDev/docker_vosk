#!/bin/bash

LD_LIBRARY_PATH=/dLabPro_vosk_api_wrapper/ VOSK_SAMPLE_RATE=48000 /dLabPro_vosk_api/bin.release/recognizer 0.0.0.0 2700 1 model_path
