#!/bin/bash

INPUTFILE=$1

echo "Creating transcript for $INPUTFILE"

rm -f ./tmp_audio_stripped.wav ./tmp_audio_stripped_resampled.wav
rm -rf tmpoutdir/
mkdir -p tmpoutdir/

ffmpeg -i $INPUTFILE ./tmp_audio_stripped.wav

sox ./tmp_audio_stripped.wav -r 48000 -c 1 -b 16 ./tmp_audio_stripped_resampled.wav

LD_LIBRARY_PATH=/home/danielzoba/evaluation_fhg_others_2025/AP1/optimization/recikts_1.1.8/:onnxruntime-linux-x64-1.12.1/lib/ \
./recikts_out/recikts_main \
/home/danielzoba/evaluation_fhg_others_2025/AP1/optimization/merged_47_nnet_v4_trns_118_wordpc_misa.cfg \
tmp_audio_stripped_resampled.wav \
tmpoutdir/

TRANSCRIPTFILE=$(echo $INPUTFILE | sed 's/\.[^./]\{3\}$/\.fhg.srt/')

echo "Writing transcript file $TRANSCRIPTFILE"

mv tmpoutdir/subtitles.srt $TRANSCRIPTFILE

TEXTFILE=$(echo $INPUTFILE | sed 's/\.[^./]\{3\}$/\.fhg.txt/')

echo "Writing transcript file $TEXTFILE"

mv tmpoutdir/transcript.txt $TEXTFILE

