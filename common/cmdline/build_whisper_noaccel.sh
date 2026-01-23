#!/bin/bash

# do this manually and only once

# git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
# cd webrtc-audio-processing && git checkout 6e37f37c4ea8790760b4c55d9ce9024a7e7bf260

# apt install -y meson libabsl-dev

# cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build



# git clone https://github.com/ZalozbaDev/whisper.cpp.git whisper.cpp

# cd whisper.cpp && git checkout v1.7.4
# cmake -B build && cmake --build build --config Release


rm -rf whisper_out/
mkdir -p whisper_out/

cp ../*.h ../*.cpp whisper_out/

cp ../../vosk_server_whisper/*.cpp ../../vosk_server_whisper/*.h whisper_out/

cp whisper.cpp/build/src/libwhisper.so.1 whisper_out/
cp whisper.cpp/build/ggml/src/*.so       whisper_out/
pushd whisper_out
ln -s libwhisper.so.1 libwhisper.so
popd

g++ -Wall -Wno-write-strings -O3 -g3 -std=c++17 -O3 -fPIC -o whisper_out/whisper_main \
-DVAD_FRAME_CONVERT_FLOAT \
-Iwhisper_out/ -I. -Iwebrtc-audio-processing/webrtc/ -Iwhisper.cpp/ -Iwhisper.cpp/examples/ \
-Iwhisper.cpp/include/ -Iwhisper.cpp/ggml/include/ \
-Ionnxruntime-linux-x64-1.12.1/include/ \
whisper_out/RecognizerBase.cpp \
whisper_out/vosk_api_wrapper.cpp whisper_out/VoskRecognizer.cpp whisper_out/VADWrapperWebRTC.cpp whisper_out/VADWrapperSilero.cpp whisper_out/SileroVadIterator.cpp whisper_out/AudioLogger.cpp \
whisper_out/ResamplerLibResample_48_16.cpp whisper_out/ResamplerWebRTC_48_16.cpp whisper_out/RepetitionRemover.cpp \
whisper_out/HunspellPostProc.cpp whisper_out/CustomPostProc.cpp \
whisper_out/WhisperImpl.cpp whisper_out/WhisperPool.cpp main.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell -licuio -licuuc -lsndfile -lonnxruntime -lresample -Lonnxruntime-linux-x64-1.12.1/lib/ -lwhisper -lggml -lggml-cpu -lggml-base -Lwhisper_out/ 

