# standalone vosk whisper server build instructions

## WEBRTC VAD lib

git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
cd webrtc-audio-processing && git checkout 8f54329708f2d5eef477b76339d44d9a31583118

brew install meson 
brew install abseil
brew install cmake

cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

## whisper.cpp dependency with COREML

git clone https://github.com/ZalozbaDev/whisper.cpp.git whisper.cpp

cd whisper.cpp && git checkout v1.7.4
cmake -B build -DWHISPER_COREML=1 && cmake --build build -j --config Release

## VOSK dependencies

git clone https://github.com/ZalozbaDev/vosk-api.git vosk-api
cd vosk-api && git checkout 1053cfa0f80039d2956de7e05a05c0b8db90c3c0

git clone https://github.com/ZalozbaDev/vosk-server.git vosk-server
cd vosk-server && git checkout 3d4ecb85bf5a8f39ead3749f49e7726fed3eed42

## VOSK server binary

brew install boost

rm -rf standalone_out && mkdir -p standalone_out

cp vosk-api/src/vosk_api.h vosk-server/websocket-cpp/asr_server.cpp standalone_out
cp whisper.cpp/build/src/*.dylib whisper.cpp/build/ggml/src/*.dylib standalone_out
cp ../common/*.h ../common/*.cpp *.h *.cpp standalone_out

cd standalone_out

g++ -std=c++17 -O3 -o vosk_whisper_server \
-DVAD_FRAME_CONVERT_FLOAT \
-DGGML_BACKEND_SHARED -DGGML_SHARED -DGGML_USE_BLAS -DGGML_USE_CPU -DGGML_USE_METAL \
-I. -I../ -I../whisper.cpp/ -I../whisper.cpp/examples/ -I../whisper.cpp/include/ -I../whisper.cpp/ggml/include/ -I../webrtc-audio-processing/webrtc/ \
-I/opt/homebrew/opt/icu4c@77/include -I/opt/homebrew/opt/hunspell/include/hunspell -I/opt/homebrew/Cellar/boost/1.88.0/include/ \
asr_server.cpp VoskRecognizer.cpp VADWrapper.cpp vosk_api_wrapper.cpp AudioLogger.cpp RecognizerBase.cpp HunspellPostProc.cpp CustomPostProc.cpp \
../webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell-1.7 -licuio -licuuc -lsndfile -lwhisper -lggml -lggml-cpu -lggml-base -L. \
-L /opt/homebrew/opt/hunspell/lib/ -L/opt/homebrew/opt/icu4c@77/lib/ -L/opt/homebrew/opt/libsndfile/lib/
