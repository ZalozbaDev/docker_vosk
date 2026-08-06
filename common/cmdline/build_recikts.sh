#!/bin/bash

# do this manually and only once

# git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
# cd webrtc-audio-processing && git checkout 6e37f37c4ea8790760b4c55d9ce9024a7e7bf260

# apt install -y meson libabsl-dev

# cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

# apt install -y libhunspell-dev  libicu-dev libsndfile1-dev libresample1-dev

rm -rf recikts_out/
mkdir -p recikts_out/

cp ../*.h ../*.cpp recikts_out/

cp ../../vosk_server_recikts/*.h ../../vosk_server_recikts/*.cpp recikts_out/

g++ -Wall -Wno-write-strings -O3 -g3 -std=c++17 -fPIC -o recikts_out/recikts_main -DPREFIX="" \
-DVAD_FRAME_CONVERT_FLOAT \
-Irecikts_out/ -I. -Iwebrtc-audio-processing/webrtc/ \
-Ionnxruntime-linux-x64-1.12.1/include/ \
recikts_out/RecognizerBase.cpp \
recikts_out/vosk_api_wrapper.cpp recikts_out/VoskRecognizer.cpp recikts_out/VADWrapperWebRTC.cpp recikts_out/VADWrapperSilero.cpp recikts_out/SileroVadIterator.cpp recikts_out/AudioLogger.cpp \
recikts_out/ResamplerLibResample_48_16.cpp recikts_out/ResamplerLibResample_8_16.cpp recikts_out/ResamplerWebRTC_48_16.cpp recikts_out/ResamplerWebRTC_8_16.cpp recikts_out/RepetitionRemover.cpp \
recikts_out/HunspellPostProc.cpp recikts_out/CustomPostProc.cpp \
recikts_out/RecIKTSImpl.cpp main.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell -licuio -licuuc -lsndfile -lonnxruntime -lresample -Lonnxruntime-linux-x64-1.12.1/lib/

