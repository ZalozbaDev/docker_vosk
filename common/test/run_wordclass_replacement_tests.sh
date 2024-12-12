#!/bin/bash

rm -f ./tests_wordclass_replacement
g++ -o tests_wordclass_replacement -g3 -Wall -I. -I.. test_support.cpp tests_wordclass_replacement.cpp ../WordClassPostProc.cpp
./tests_wordclass_replacement

