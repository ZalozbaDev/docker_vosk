#!/bin/bash

rm -rf tmpout/
mkdir -p tmpout/

python3.11 -m demucs.separate -o tmpout/ \
--device cpu --shifts 1 --overlap 0.25 -j 4 --two-stems vocals \
--mp3 --mp3-bitrate 320 --mp3-preset 2 "$1"

