#!/bin/bash

rm -f ./tests_custompostproc
g++ -o tests_custompostproc -g3 -Wall -I. -I.. test_support.cpp tests_custompostproc.cpp ../CustomPostProc.cpp ../RepetitionRemover.cpp -licuio -licuuc
./tests_custompostproc

rm -f ./tests_repetitionremover
g++ -o tests_repetitionremover -g3 -Wall -I. -I.. test_support.cpp tests_repetitionremover.cpp ../RepetitionRemover.cpp 
./tests_repetitionremover

rm -f ./tests_postproc
g++ -o tests_postproc -g3 -Wall -DTEST_HUNSPELL -I. -I.. test_support.cpp tests_hunspell.cpp ../HunspellPostProc.cpp -lhunspell
./tests_postproc

