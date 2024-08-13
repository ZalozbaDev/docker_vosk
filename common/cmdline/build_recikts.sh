#!/bin/bash

# do this manually and only once

# git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
# cd webrtc-audio-processing && git checkout 6e37f37c4ea8790760b4c55d9ce9024a7e7bf260

# apt install -y meson libabsl-dev

# cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

rm -rf recikts_out/
mkdir -p recikts_out/

cp ../*.h ../*.cpp recikts_out/

cp ../../vosk_server_recikts/recikts.h ../../vosk_server_recikts/VoskRecognizer.cpp ../../vosk_server_recikts/VoskRecognizer.h recikts_out/

g++ -Wall -Wno-write-strings -g3 -std=c++17 -O3 -fPIC -o recikts_out/recikts_main -DPREFIX="" \
-Irecikts_out/ -I. -Iwebrtc-audio-processing/webrtc/ \
recikts_out/vosk_api_wrapper.cpp recikts_out/VoskRecognizer.cpp recikts_out/VADWrapper.cpp recikts_out/AudioLogger.cpp \
recikts_out/HunspellPostProc.cpp recikts_out/CustomPostProc.cpp \
main.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell -licuio -licuuc -lsndfile

