#!/bin/bash

rm -f ./tests_voskrecognizer_whisper
g++ -o tests_voskrecognizer_whisper -g3 -Wall -DWEBRTC_VAD_MOCK -DSIGNAL_PROCESSING_MOCK -DWHISPER_MOCK -DTEST_VADWRAPPER -DVAD_FRAME_CONVERT_FLOAT	\
-I. -I.. -I../../vosk_server_whisper/ \
test_support.cpp tests_voskrecognizer_whisper.cpp webrtc_vad_mock.c signal_processing_mock.c whisper_mock.cpp ../VADWrapper.cpp ../AudioLogger.cpp ../HunspellPostProc.cpp ../CustomPostProc.cpp \
../../vosk_server_whisper/VoskRecognizer.cpp -lhunspell -licuio -licuuc
./tests_voskrecognizer_whisper
