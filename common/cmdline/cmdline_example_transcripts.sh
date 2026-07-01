#!/bin/bash

INPUTFILE=$1

echo "Creating transcript for $INPUTFILE"

rm -f ./tmp_audio_stripped.wav ./tmp_audio_stripped_resampled.wav
rm -rf tmpoutdir/
mkdir -p tmpoutdir/

ffmpeg -i "$INPUTFILE" ./tmp_audio_stripped.wav

sox ./tmp_audio_stripped.wav -r 48000 -c 1 -b 16 ./tmp_audio_stripped_resampled.wav

LD_LIBRARY_PATH=whisper.cpp/build/src/:onnxruntime-linux-x64-1.12.1/lib/ ./whisper_out/whisper_main \
~/whisper_models/zalozbadev/whisper_large_v3_turbo_hsb_aug/ggml-model.bin \
tmp_audio_stripped_resampled.wav \
tmpoutdir/ \
2 czech -1 false WebRTC -1.0 false \
./hsb_DE_soblex_w8_3.09.11.aff ./hsb_DE_soblex_w8_3.09.11.dic

TRANSCRIPTFILE=$(echo $INPUTFILE | sed 's/\.[^./]\{3\}$/\.srt/')

echo "Writing transcript file $TRANSCRIPTFILE"

mv tmpoutdir/subtitles.srt "$TRANSCRIPTFILE"

TEXTFILE=$(echo $INPUTFILE | sed 's/\.[^./]\{3\}$/\.txt/')

echo "Writing transcript file $TEXTFILE"

mv tmpoutdir/transcript.txt "$TEXTFILE"

