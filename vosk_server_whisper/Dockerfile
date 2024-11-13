FROM ubuntu:jammy
MAINTAINER Daniel Sobe <daniel.sobe@sorben.com>

# normal call
# docker build -f vosk_server_whisper/Dockerfile --progress=plain -t vosk_server_whisper .

# rebuild from scratch
# docker build -f vosk_server_whisper/Dockerfile --progress=plain -t vosk_server_whisper . --no-cache

# RUN sed -i 's/ main/ main contrib non-free/' /etc/apt/sources.list

RUN apt update

########################################################
# Setup locale to properly support sorbian diacritics
########################################################

RUN apt-get install -y locales

RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    locale-gen

ENV LC_ALL en_US.UTF-8 
ENV LANG en_US.UTF-8  
ENV LANGUAGE en_US:en     

########################################################
# Install all dependencies for compilation
########################################################

RUN apt install -y g++ wget git make nano 

###################################
# Build WebRTC dependency
###################################

RUN git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
RUN cd webrtc-audio-processing && git checkout 6e37f37c4ea8790760b4c55d9ce9024a7e7bf260

RUN apt install -y meson libabsl-dev

RUN cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

############################################
# Build vosk server wrapper and whisper glue code
############################################


RUN apt install -y nvidia-cuda-toolkit 

# try to force a fixed version
RUN apt install -y nvidia-utils-535-server=535.216.01-0ubuntu0.22.04.1

# get boost source code for some boost header files 
RUN wget -q https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz \
  && tar xzf boost_1_76_0.tar.gz

# get VOSK API (for header file)
RUN git clone https://github.com/ZalozbaDev/vosk-api.git vosk-api
RUN cd vosk-api && git checkout 1053cfa0f80039d2956de7e05a05c0b8db90c3c0

# get VOSK server implementation
RUN git clone https://github.com/ZalozbaDev/vosk-server.git vosk-server
RUN cd vosk-server && git checkout 21147c33e383f45b942846c9f713789d8bca41d1

RUN cp vosk-api/src/vosk_api.h /
RUN cp vosk-server/websocket-cpp/asr_server.cpp /

# dummy to trigger re-fetch
RUN touch AAA

# get whisper.cpp files
RUN git clone https://github.com/ZalozbaDev/whisper.cpp.git whisper.cpp
RUN cd whisper.cpp && git checkout v1.7.1

# use this line to test other commits
# RUN cd whisper.cpp && git checkout f9ca90256bf6

# assure all cuda dependencies are built
RUN cd whisper.cpp/ && GGML_CUDA=1 CUDA_DOCKER_ARCH=sm_61 make main

RUN apt install -y libhunspell-dev  libicu-dev

COPY common/AudioLogger.cpp  common/AudioLogger.h  common/RecognizerBase.cpp \
common/RecognizerBase.h common/RecognitionResult.h  common/VADFrame.h  \
common/VADWrapper.cpp  common/VADWrapper.h  common/vosk_api_wrapper.cpp \
common/HunspellPostProc.h common/HunspellPostProc.cpp common/CustomPostProc.cpp \
common/CustomPostProc.h /
COPY vosk_server_whisper/VoskRecognizer.cpp vosk_server_whisper/VoskRecognizer.h /

# RUN find /whisper.cpp/ -name "*.o"

RUN g++ -std=c++17 -O3 -o vosk_whisper_server \
-Wall -Wextra -Wpedantic -Wcast-qual -Wno-unused-function -Wno-unused-parameter -Wno-missing-declarations -Wmissing-noreturn -Wno-write-strings -Wno-array-bounds -Wno-format-truncation -Wextra-semi \
-D_XOPEN_SOURCE=600 -D_GNU_SOURCE -DNDEBUG -DGGML_USE_OPENMP -DGGML_USE_CUDA \
-pthread -fopenmp -fPIC -march=native -mtune=native \ 
-DVAD_FRAME_CONVERT_FLOAT \
-I/boost_1_76_0/ -I. -I/whisper.cpp/ -I/whisper.cpp/examples/ -I/whisper.cpp/include/ -I/whisper.cpp/ggml/include/ -I/webrtc-audio-processing/webrtc/ \
asr_server.cpp VoskRecognizer.cpp VADWrapper.cpp vosk_api_wrapper.cpp AudioLogger.cpp RecognizerBase.cpp HunspellPostProc.cpp CustomPostProc.cpp \
whisper.cpp/examples/common.cpp whisper.cpp/examples/common-ggml.cpp  whisper.cpp/ggml/src/ggml.o whisper.cpp/src/whisper.o \
whisper.cpp/ggml/src/ggml-cuda.o whisper.cpp/ggml/src/ggml-alloc.o whisper.cpp/ggml/src/ggml-backend.o whisper.cpp/ggml/src/ggml-quants.o whisper.cpp/ggml/src/ggml-aarch64.o \
whisper.cpp/ggml/src/ggml-cuda/*.o whisper.cpp/ggml/src/ggml-cuda/template-instances/*.o \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-lpthread -lhunspell  -licuio -licuuc -lcuda -lcublas -lculibos -lcudart -lcublasLt -L/usr/local/cuda/lib64 -L/opt/cuda/lib64

RUN mkdir -p /logs/
RUN mkdir -p /whisper/

RUN apt install -y gdb

############################################
# Vosk server startup script
############################################

COPY vosk_server_whisper/startme.sh /
RUN chmod 755 startme.sh

CMD ["/startme.sh"]

# CMD ["./asr_server", "0.0.0.0" ,"2700", "5" ,"/opt/vosk-model-en/model"]
