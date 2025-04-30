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


