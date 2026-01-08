#!/bin/bash

BINARY=binary_tests_whisper_pool

rm -f ./${BINARY}
g++ -o ${BINARY} -g3 -Wall -DWHISPER_MOCK \
-I. -I.. -I../../vosk_server_whisper/ \
test_support.cpp tests_whisper_pool.cpp whisper_mock.cpp \
../../vosk_server_whisper/WhisperPool.cpp ../../vosk_server_whisper/WhisperImpl.cpp -lhunspell -licuio -licuuc
./${BINARY}
