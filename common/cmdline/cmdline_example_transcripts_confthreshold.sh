#!/bin/bash

INPUTFILE=$1
CONF=$2

echo "Creating transcript for $INPUTFILE"

rm -f ./tmp_audio_stripped.wav ./tmp_audio_stripped_resampled.wav
rm -rf tmpoutdir/
mkdir -p tmpoutdir/

ffmpeg -i $INPUTFILE ./tmp_audio_stripped.wav

sox ./tmp_audio_stripped.wav -r 48000 -c 1 -b 16 ./tmp_audio_stripped_resampled.wav

LD_LIBRARY_PATH=whisper.cpp/build/src/:onnxruntime-linux-x64-1.12.1/lib/ ./whisper_out/whisper_main \
../../../whisper_models/Korla/whisper_large_v3_turbo_hsb/ggml-model.bin \
tmp_audio_stripped_resampled.wav \
tmpoutdir/ 2 auto 0 false Silero $CONF

TRANSCRIPTFILE=$(echo $INPUTFILE | sed "s/\.[^./]\{3\}$/-${CONF}\.srt/")

echo "Writing transcript file $TRANSCRIPTFILE"

mv tmpoutdir/subtitles.srt $TRANSCRIPTFILE

