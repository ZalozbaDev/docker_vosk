# this dockerfile is ment to be build from the github.com/ZalozbaDev/mudrowak repository
# docker_vosk (this repo) and all dependencies are included in mudrowak as submodules
# for easiest setup of everything run `docker-compose up -d --build` inside mudrowak root

FROM ghcr.io/ggerganov/whisper.cpp:main as whispercpp

FROM ubuntu:24.04 as build
WORKDIR /opt

RUN apt-get update && apt-get install -y g++ wget make meson libabsl-dev libboost1.83-dev

# copy and build webrtc-audio-processing
COPY ./webrtc-audio-processing ./webrtc-audio-processing
RUN cd webrtc-audio-processing \
    && meson . build -Dprefix=$PWD/install \
    && ninja -C build

# copy whisper.cpp pre-built binaries from upstread docker image import above
COPY --from=whispercpp /app ./whisper.cpp

# copy sources of vosk-api, vosk-server and docker_vosk
COPY ./vosk-api/src/vosk_api.h ./
COPY ./vosk-server/websocket-cpp/asr_server.cpp ./
COPY docker_vosk/common/*.cpp docker_vosk/common/*.h ./
COPY docker_vosk/vosk_server_whisper/*.cpp docker_vosk/vosk_server_whisper/*.h ./

RUN g++ -Wall -Wno-write-strings -std=c++17 -O3 -fPIC -DGGML_USE_CUBLAS \
    -o vosk_whisper_server -DVAD_FRAME_CONVERT_FLOAT -I./whisper.cpp/ \
    -I./whisper.cpp/examples/ -I./webrtc-audio-processing/webrtc/ -I./ \
    asr_server.cpp VoskRecognizer.cpp VADWrapper.cpp vosk_api_wrapper.cpp \
    AudioLogger.cpp \
    whisper.cpp/examples/common.cpp whisper.cpp/examples/common-ggml.cpp \
    whisper.cpp/ggml.o whisper.cpp/whisper.o whisper.cpp/ggml-alloc.o \
    whisper.cpp/ggml-backend.o whisper.cpp/ggml-quants.o \
    webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
    -lpthread

RUN mkdir ./logs ./whisper

CMD ["./asr_server", "0.0.0.0" ,"2700", "1" ,"./ggml-model.q8_0.bin"]
