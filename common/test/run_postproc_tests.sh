#!/bin/bash

rm -f ./tests_postproc
g++ -o tests_postproc -g3 -Wall -DTEST_HUNSPELL -I. -I.. test_support.cpp tests_hunspell.cpp ../HunspellPostProc.cpp -lhunspell
./tests_postproc
