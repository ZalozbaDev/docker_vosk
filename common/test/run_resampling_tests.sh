#!/bin/bash

# git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
# cd webrtc-audio-processing && git checkout 001d483d0c761f004e1888307acccc87d3520ec8
# meson . build -Dprefix=$PWD/install && ninja -C build

rm -f ./tests_resampling 
g++ -o tests_resampling -fprofile-arcs -ftest-coverage -g3 -Wall -Wextra -Wno-unused -I. -I.. test_support.cpp tests_resampling.cpp \
../ResamplerWebRTC_48_16.cpp   ../ResamplerWebRTC_8_16.cpp \
-I webrtc-audio-processing/webrtc/ webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a
./tests_resampling


#gcov -o tests_vadwrapper-VADWrapper -s ../ VADWrapper.cpp 
#gcovr --html-details --output index.html --root ../

#echo "open index.html for browsing results!"

#rm -f ./*.gcda ./*.gcno ./*.gcov