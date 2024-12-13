#!/bin/bash

rm -f ./tests_vadwrapper ./*.gcda ./*.gcno ./*.gcov
g++ -o tests_vadwrapper -fprofile-arcs -ftest-coverage -g3 -Wall -Wextra -Wno-unused -DWEBRTC_VAD_MOCK -DTEST_VADWRAPPER -I. -I.. test_support.cpp tests_vadwrapper.cpp webrtc_vad_mock.c ../VADWrapper.cpp
./tests_vadwrapper
gcov -o tests_vadwrapper-VADWrapper -s ../ VADWrapper.cpp 
gcovr --html-details --output index.html --root ../

echo "open index.html for browsing results!"

rm -f ./*.gcda ./*.gcno ./*.gcov