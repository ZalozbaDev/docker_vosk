ffmpeg -i __INPUT_FILM__ audio_stripped.wav

sox audio_stripped.wav -r 48000 -c 1 -b 16 audio_resampled.wav

mkdir -p __OUTPUT_DIR__MUST_EXIST__

LD_LIBRARY_PATH=whisper.cpp/build/src/ ./whisper_out/whisper_main \
../../../whisper_models/Korla/whisper_large_v3_turbo_hsb/ggml-model.bin \
audio_resampled.wav \
__OUTPUT_DIR__MUST_EXIST__
