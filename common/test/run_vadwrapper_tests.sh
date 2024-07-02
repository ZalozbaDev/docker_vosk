#!/bin/bash

rm -f ./tests_vadwrapper
g++ -o tests_vadwrapper -g3 -Wall -Wextra -Wno-unused -DWEBRTC_VAD_MOCK -DTEST_VADWRAPPER -I. -I.. test_support.cpp tests_vadwrapper.cpp webrtc_vad_mock.c ../VADWrapper.cpp
./tests_vadwrapper
