#!/bin/bash

# do this manually and only once

# git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
# cd webrtc-audio-processing && git checkout 6e37f37c4ea8790760b4c55d9ce9024a7e7bf260

# apt install -y meson libabsl-dev

# cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build



# git clone https://github.com/ZalozbaDev/whisper.cpp.git whisper.cpp
# cd whisper.cpp && git checkout v1.6.2

# make main


rm -rf whisper_out/
mkdir -p whisper_out/

cp ../*.h ../*.cpp whisper_out/

cp ../../vosk_server_whisper/VoskRecognizer.cpp ../../vosk_server_whisper/VoskRecognizer.h whisper_out/

g++ -Wall -Wno-write-strings -O3 -g3 -std=c++17 -O3 -fPIC -o whisper_out/whisper_main \
-DVAD_FRAME_CONVERT_FLOAT \
-Iwhisper_out/ -I. -Iwebrtc-audio-processing/webrtc/ -Iwhisper.cpp/ -Iwhisper.cpp/examples/ \
whisper_out/RecognizerBase.cpp \
whisper_out/vosk_api_wrapper.cpp whisper_out/VoskRecognizer.cpp whisper_out/VADWrapper.cpp whisper_out/AudioLogger.cpp \
whisper_out/HunspellPostProc.cpp whisper_out/CustomPostProc.cpp \
whisper.cpp/examples/common.cpp whisper.cpp/examples/common-ggml.cpp whisper.cpp/ggml.o whisper.cpp/whisper.o \
whisper.cpp/ggml-alloc.o whisper.cpp/ggml-backend.o whisper.cpp/ggml-quants.o \
main.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell -licuio -licuuc -lsndfile

