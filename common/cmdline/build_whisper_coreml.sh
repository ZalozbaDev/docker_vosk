
# do this manually and only once

# git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
# cd webrtc-audio-processing && git checkout 001d483d0c761f004e1888307acccc87d3520ec8

# brew install meson 
# brew install abseil
# brew install cmake

# cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build


# install CMake (via GUI or homebrew) and make sure that XCode cmdline utils are up to date

# git clone https://github.com/ZalozbaDev/whisper.cpp.git whisper.cpp

# cd whisper.cpp && git checkout v1.7.4
# cmake -B build -DWHISPER_COREML=1 && cmake --build build -j --config Release

rm -rf whisper_out/
mkdir -p whisper_out/

cp ../*.h ../*.cpp whisper_out/

cp ../../vosk_server_whisper/VoskRecognizer.cpp ../../vosk_server_whisper/VoskRecognizer.h whisper_out/

# cp whisper.cpp/build/src/libwhisper.so.1 whisper_out/
# cp whisper.cpp/build/ggml/src/*.so       whisper_out/
# pushd whisper_out
# ln -s libwhisper.so.1 libwhisper.so
# popd

g++ -Wall -Wno-write-strings -O3 -g3 -std=c++17 -O3 -fPIC -o whisper_out/whisper_main \
-DVAD_FRAME_CONVERT_FLOAT \
-Iwhisper_out/ -I. -Iwebrtc-audio-processing/webrtc/ -Iwhisper.cpp/ -Iwhisper.cpp/examples/ \
-Iwhisper.cpp/include/ -Iwhisper.cpp/ggml/include/ \
whisper_out/RecognizerBase.cpp \
whisper_out/vosk_api_wrapper.cpp whisper_out/VoskRecognizer.cpp whisper_out/VADWrapper.cpp whisper_out/AudioLogger.cpp \
whisper_out/HunspellPostProc.cpp whisper_out/CustomPostProc.cpp \
main.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell -licuio -licuuc -lsndfile -lwhisper -lggml -lggml-cpu -lggml-base -Lwhisper_out/ 

