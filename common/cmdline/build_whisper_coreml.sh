
# do this manually and only once

# git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
# cd webrtc-audio-processing && git checkout 8f54329708f2d5eef477b76339d44d9a31583118

# brew install meson 
# brew install abseil
# brew install cmake

# cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

# [install CMake (via GUI or homebrew)] and make sure that XCode cmdline utils are up to date

# git clone https://github.com/ZalozbaDev/whisper.cpp.git whisper.cpp

# cd whisper.cpp && git checkout v1.7.4
# cmake -B build -DWHISPER_COREML=1 && cmake --build build -j --config Release

# brew install libsndfile
# brew install hunspell
# brew install icu4c

rm -rf whisper_out/
mkdir -p whisper_out/

cp ../*.h ../*.cpp whisper_out/

cp ../../vosk_server_whisper/VoskRecognizer.cpp ../../vosk_server_whisper/VoskRecognizer.h whisper_out/

cp whisper.cpp/build/src/*.dylib       whisper_out/
cp whisper.cpp/build/ggml/src/*.dylib  whisper_out/
# pushd whisper_out
# ln -s libwhisper.so.1 libwhisper.so
# popd

g++ -Wall -Wno-write-strings -O3 -g3 -std=c++17 -O3 -fPIC -o whisper_out/whisper_main \
-DVAD_FRAME_CONVERT_FLOAT \
-I/opt/homebrew/opt/icu4c@77/include -I/opt/homebrew/opt/hunspell/include/hunspell -I/opt/homebrew/opt/libsndfile/include \
-Iwhisper_out/ -I. -Iwebrtc-audio-processing/webrtc/ -Iwhisper.cpp/ -Iwhisper.cpp/examples/ \
-Iwhisper.cpp/include/ -Iwhisper.cpp/ggml/include/ \
whisper_out/RecognizerBase.cpp \
whisper_out/vosk_api_wrapper.cpp whisper_out/VoskRecognizer.cpp whisper_out/VADWrapper.cpp whisper_out/AudioLogger.cpp \
whisper_out/HunspellPostProc.cpp whisper_out/CustomPostProc.cpp \
main.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell-1.7 -licuio -licuuc -lsndfile -lwhisper -lggml -lggml-cpu -lggml-base -Lwhisper_out/ \
-L /opt/homebrew/opt/hunspell/lib/ -L/opt/homebrew/opt/icu4c@77/lib/ -L/opt/homebrew/opt/libsndfile/lib/

# example execution

# DYLD_LIBRARY_PATH=./whisper_out/ ./whisper_out/whisper_main
