# docker build -f common/cmdline/docker/Dockerfile -t reco_deps .

################ build container for whisper.cpp / docker_vosk cmdline
FROM nvidia/cuda:13.1.1-cudnn-devel-ubuntu24.04 AS whisper_builder

WORKDIR /opt/build

RUN apt update && apt install -y libhunspell-dev libicu-dev libsndfile1-dev build-essential zlib1g-dev meson libabsl-dev \
    libncurses5-dev libgdbm-dev libnss3-dev libssl-dev libreadline-dev libffi-dev libsqlite3-dev wget libbz2-dev git cmake \
    libboost-all-dev libresample1-dev && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/ZalozbaDev/whisper.cpp.git
RUN cd whisper.cpp && git checkout v1.8.6

ENV CUDA_MAIN_VERSION=13.1
ENV LD_LIBRARY_PATH=/usr/local/cuda-${CUDA_MAIN_VERSION}/compat:$LD_LIBRARY_PATH
RUN cd whisper.cpp/ && cmake -B build -DGGML_CUDA=1 -DCMAKE_CUDA_ARCHITECTURES=89 && cmake --build build -j4 --config Release

COPY common common
COPY vosk_server_recikts vosk_server_recikts
COPY vosk_server_whisper vosk_server_whisper

RUN cd common/cmdline/ && git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git \
    && cd webrtc-audio-processing && git checkout v2.1 \ 
    && meson . build -Dprefix=$PWD/install && ninja -C build

RUN cd common/cmdline/ && cp -r ../../whisper.cpp .

# get ONNX runtime (for Silero)
RUN wget https://github.com/microsoft/onnxruntime/releases/download/v1.12.1/onnxruntime-linux-x64-1.12.1.tgz
RUN mv onnxruntime-linux-x64-1.12.1.tgz common/cmdline/ && cd common/cmdline/ && tar xvfz onnxruntime-linux-x64-1.12.1.tgz

RUN cd common/cmdline/ && ./build_recikts.sh
RUN cd common/cmdline/ && ./build_whisper_nvidia.sh

