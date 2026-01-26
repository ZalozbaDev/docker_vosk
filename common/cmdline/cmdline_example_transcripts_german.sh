#!/bin/bash

echo "Using this model: https://huggingface.co/cstr/whisper-large-v3-turbo-german-ggml"

INPUTFILE=$1

echo "Creating transcript for $INPUTFILE"

rm -f ./tmp_audio_stripped.wav ./tmp_audio_stripped_resampled.wav
rm -rf tmpoutdir/
mkdir -p tmpoutdir/

ffmpeg -i $INPUTFILE ./tmp_audio_stripped.wav

sox ./tmp_audio_stripped.wav -r 48000 -c 1 -b 16 ./tmp_audio_stripped_resampled.wav

export VOSK_MODEL_LANGUAGE=de

LD_LIBRARY_PATH=whisper.cpp/build/src/:onnxruntime-linux-x64-1.12.1/lib/ ./whisper_out/whisper_main \
~/whisper_models/primeline/whisper-large-v3-german/ggml-model.bin tmp_audio_stripped_resampled.wav \
tmpoutdir/

TRANSCRIPTFILE=$(echo $INPUTFILE | sed 's/\.[^./]\{3\}$/\.de.srt/')

echo "Writing transcript file $TRANSCRIPTFILE"

mv tmpoutdir/subtitles.srt $TRANSCRIPTFILE

TEXTFILE=$(echo $INPUTFILE | sed 's/\.[^./]\{3\}$/\.de.txt/')

echo "Writing transcript file $TEXTFILE"

mv tmpoutdir/transcript.txt $TEXTFILE

