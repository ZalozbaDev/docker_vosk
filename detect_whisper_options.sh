#!/bin/bash

# pakćik za čěrjak

echo
echo "driver package + version"
echo "========================"
dpkg -la | grep ii | grep nvidia | grep driver
echo

echo "compile and run helper program to detect CUDA architecture"
echo "=========================================================="
nvcc detect_arch.cu -o detect_arch
./detect_arch
echo

echo "fill values into this build cmdline"
echo "==================================="
head -n 5 vosk_server_whisper/Dockerfile

