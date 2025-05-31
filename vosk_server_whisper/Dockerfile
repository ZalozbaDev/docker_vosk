FROM ubuntu:noble
MAINTAINER Daniel Sobe <daniel.sobe@sorben.com>

# normal call
# docker build -f vosk_server_whisper/Dockerfile --build-arg NVIDIA_DRIVER_PACKAGE= --build-arg NVIDIA_DRIVER_VERSION= --build-arg NVIDIA_CUDA_ARCHITECTURE= --progress=plain -t vosk_server_whisper .

# rebuild from scratch
# docker build -f vosk_server_whisper/Dockerfile --build-arg NVIDIA_DRIVER_PACKAGE= --build-arg NVIDIA_DRIVER_VERSION= --build-arg NVIDIA_CUDA_ARCHITECTURE= --progress=plain -t vosk_server_whisper . --no-cache

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
RUN cd webrtc-audio-processing && git checkout 001d483d0c761f004e1888307acccc87d3520ec8

RUN apt install -y meson libabsl-dev

RUN cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

############################################
# Create environment for building with CUDA10
############################################

RUN wget https://ftp.gnu.org/gnu/gcc/gcc-8.5.0/gcc-8.5.0.tar.xz
RUN tar xJf gcc-8.5.0.tar.xz
RUN apt install -y libgmp-dev libmpfr-dev libmpc-dev
RUN cd gcc-8.5.0/ && ./configure --disable-multilib --disable-bootstrap --enable-languages=c,c++ --prefix=/opt/gcc-8/
RUN cd gcc-8.5.0/ && make -j 4 && make install


RUN wget https://developer.download.nvidia.com/compute/cuda/10.2/Prod/local_installers/cuda_10.2.89_440.33.01_linux.run
RUN wget https://developer.download.nvidia.com/compute/cuda/10.2/Prod/patches/1/cuda_10.2.1_linux.run
RUN wget https://developer.download.nvidia.com/compute/cuda/10.2/Prod/patches/2/cuda_10.2.2_linux.run

RUN chmod 755 cuda_10.2.89_440.33.01_linux.run && sh cuda_10.2.89_440.33.01_linux.run --silent --toolkit --override --no-drm
RUN chmod 755 cuda_10.2.1_linux.run && sh cuda_10.2.1_linux.run --silent --toolkit --override --no-drm
RUN chmod 755 cuda_10.2.2_linux.run && sh cuda_10.2.2_linux.run --silent --toolkit --override --no-drm

RUN cd /usr/local/cuda-10.2/bin/ && ln -s /opt/gcc-8/bin/gcc gcc && ln -s /opt/gcc-8/bin/g++ g++

############################################
# Build vosk server wrapper and whisper glue code
############################################


RUN apt install -y nvidia-cuda-toolkit 

ARG NVIDIA_DRIVER_PACKAGE
ARG NVIDIA_DRIVER_VERSION
ARG NVIDIA_CUDA_ARCHITECTURE 

# try to force a fixed version
RUN apt install -y $NVIDIA_DRIVER_PACKAGE=$NVIDIA_DRIVER_VERSION

# get boost source code for some boost header files 
#RUN wget -q https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz \
#  && tar xzf boost_1_76_0.tar.gz
RUN apt install -y libboost-all-dev

# get VOSK API (for header file)
RUN git clone https://github.com/ZalozbaDev/vosk-api.git vosk-api
RUN cd vosk-api && git checkout 1053cfa0f80039d2956de7e05a05c0b8db90c3c0

# get VOSK server implementation
RUN git clone https://github.com/ZalozbaDev/vosk-server.git vosk-server
RUN cd vosk-server && git checkout 21147c33e383f45b942846c9f713789d8bca41d1

RUN cp vosk-api/src/vosk_api.h /
RUN cp vosk-server/websocket-cpp/asr_server.cpp /

# get whisper.cpp files
RUN git clone https://github.com/danielzoba/whisper.cpp.git whisper.cpp
RUN cd whisper.cpp && git checkout v1.7.4_CUDA10

# use this line to test other commits
# RUN cd whisper.cpp && git checkout f9ca90256bf6

# assure all cuda dependencies are built
# -DCUDA_DOCKER_ARCH=sm_61 
# TBD how to specify???
RUN cd whisper.cpp/ && export PATH=/usr/local/cuda-10.2/bin/:$PATH && export LD_LIBRARY_PATH=/usr/local/cuda-10.2/lib64/:$LD_LIBRARY_PATH && \
cmake -B build -DGGML_CUDA=1 -DCMAKE_CUDA_ARCHITECTURES=$NVIDIA_CUDA_ARCHITECTURE -DCMAKE_CUDA_STANDARD=14 -DCMAKE_CUDA_STANDARD_REQUIRED=true && cmake --build build -j 4 --config Release

RUN apt install -y libhunspell-dev  libicu-dev

COPY common/AudioLogger.cpp  common/AudioLogger.h  common/RecognizerBase.cpp \
common/RecognizerBase.h common/RecognitionResult.h  common/VADFrame.h  \
common/VADWrapper.cpp  common/VADWrapper.h  common/vosk_api_wrapper.cpp \
common/HunspellPostProc.h common/HunspellPostProc.cpp common/CustomPostProc.cpp \
common/CustomPostProc.h /
COPY vosk_server_whisper/VoskRecognizer.cpp vosk_server_whisper/VoskRecognizer.h /

# RUN find /whisper.cpp/ -name "*.o"

RUN cp whisper.cpp/build/src/libwhisper.so.1 /
RUN cp whisper.cpp/build/ggml/src/*.so       /
RUN ln -s libwhisper.so.1 libwhisper.so

RUN g++ -std=c++17 -O3 -o vosk_whisper_server \
-Wall -Wextra -Wpedantic -Wcast-qual -Wno-unused-function -Wno-unused-parameter -Wno-missing-declarations -Wmissing-noreturn -Wno-write-strings -Wno-array-bounds -Wno-format-truncation -Wextra-semi \
-D_XOPEN_SOURCE=600 -D_GNU_SOURCE -DNDEBUG -DGGML_USE_OPENMP -DGGML_USE_CUDA \
-pthread -fopenmp -fPIC -march=native -mtune=native \ 
-DVAD_FRAME_CONVERT_FLOAT \
-I. -I/whisper.cpp/ -I/whisper.cpp/examples/ -I/whisper.cpp/include/ -I/whisper.cpp/ggml/include/ -I/webrtc-audio-processing/webrtc/ \
asr_server.cpp VoskRecognizer.cpp VADWrapper.cpp vosk_api_wrapper.cpp AudioLogger.cpp RecognizerBase.cpp HunspellPostProc.cpp CustomPostProc.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-lpthread -lhunspell  -licuio -licuuc -lwhisper -lggml -lggml-cpu -lggml-base -L/

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
