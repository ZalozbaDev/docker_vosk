#!/bin/bash

echo "Using this model: https://huggingface.co/cstr/whisper-large-v3-turbo-german-ggml"

INPUTFILE=$1

echo "Creating transcript for $INPUTFILE"

rm -f ./tmp_audio_stripped.wav ./tmp_audio_stripped_resampled.wav
rm -rf tmpoutdir/
mkdir -p tmpoutdir/

ffmpeg -i $INPUTFILE ./tmp_audio_stripped.wav

sox ./tmp_audio_stripped.wav -r 48000 -c 1 -b 16 ./tmp_audio_stripped_resampled.wav

# export VOSK_MODEL_LANGUAGE=de

# custom finetuned de model
if /bin/false; then
    LD_LIBRARY_PATH=whisper.cpp/build/src/:onnxruntime-linux-x64-1.12.1/lib/ ./whisper_out/whisper_main \
    ~/whisper_models/primeline/whisper-large-v3-german/ggml-model.bin tmp_audio_stripped_resampled.wav \
    tmpoutdir/ \
    2 de -1 false Silero -1.0 false
fi

# whisper large v3 turbo
if /bin/true; then
    LD_LIBRARY_PATH=whisper.cpp/build/src/:onnxruntime-linux-x64-1.12.1/lib/ ./whisper_out/whisper_main \
    ~/whisper_models/openai/whisper_large_v3_turbo/ggml-model.bin tmp_audio_stripped_resampled.wav \
    tmpoutdir/ \
    2 de -1 false Silero -1.0 false
fi

if /bin/false; then
    sox ./tmp_audio_stripped.wav -r 16000 -c 1 -b 16 ./tmp_audio_stripped_resampled.wav
    LD_LIBRARY_PATH=whisper.cpp/build/src/ whisper.cpp/build/bin/whisper-cli -m ~/whisper_models/openai/whisper_large_v3_turbo/ggml-model.bin -f tmp_audio_stripped_resampled.wav --output-txt --output-file tmp_audio_stripped_resampled
fi

TRANSCRIPTFILE=$(echo $INPUTFILE | sed 's/\.[^./]\{3\}$/\.de.srt/')

echo "Writing transcript file $TRANSCRIPTFILE"

mv tmpoutdir/subtitles.srt $TRANSCRIPTFILE

TEXTFILE=$(echo $INPUTFILE | sed 's/\.[^./]\{3\}$/\.de.txt/')

echo "Writing transcript file $TEXTFILE"

mv tmpoutdir/transcript.txt $TEXTFILE

