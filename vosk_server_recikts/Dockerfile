FROM debian:bookworm-slim
MAINTAINER Daniel Sobe <daniel.sobe@sorben.com>

# normal call
# docker build -f vosk_server_recikts/Dockerfile --progress=plain -t vosk_server_recikts .

# rebuild from scratch
# docker build -f vosk_server_recikts/Dockerfile --progress=plain -t vosk_server_recikts . --no-cache

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
# RUN cd webrtc-audio-processing && git checkout working_2022_2026
RUN cd webrtc-audio-processing && git checkout v2.1

RUN apt install -y meson libabsl-dev

RUN cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

############################################
# Build vosk server wrapper and recognizer
############################################

# get boost source code for some boost header files 
#RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz \
#  && tar xzf boost_1_76_0.tar.gz
# alternative: use the debian packages, reduces external dependencies
RUN apt install -y libboost-all-dev libhunspell-dev libicu-dev libresample1-dev gdb 

# get VOSK API (for header file)
RUN git clone https://github.com/ZalozbaDev/vosk-api.git vosk-api
RUN cd vosk-api && git checkout 7ac989311dd23d4d9e3e12de170f076eb5b77be6

# get VOSK server implementation
RUN git clone https://github.com/ZalozbaDev/vosk-server.git vosk-server
RUN cd vosk-server && git checkout aeb031987fb948941933936ffc802bf9c00ac461

# get ONNX runtime (for Silero)
RUN wget https://github.com/microsoft/onnxruntime/releases/download/v1.12.1/onnxruntime-linux-x64-1.12.1.tgz
RUN tar xvfz onnxruntime-linux-x64-1.12.1.tgz

RUN cp    vosk-api/src/vosk_api.h   /
RUN cp    vosk-server/websocket-cpp/asr_server.cpp vosk-server/websocket-cpp/vosk_commands.* /
RUN cp -r vosk-server/websocket-cpp/nlohmann/json/single_include/nlohmann                    /

COPY common/*.cpp  common/*.h /
COPY vosk_server_recikts/*.h vosk_server_recikts/*.cpp /

RUN mkdir -p /data/

RUN g++ -Wall -Wno-write-strings -std=c++17 -O3 -fPIC -o recikts_server -I. \
-DVAD_FRAME_CONVERT_FLOAT \
-I/webrtc-audio-processing/webrtc/ -DPREFIX=\"/data/\" \
-I/onnxruntime-linux-x64-1.12.1/include/ \
asr_server.cpp vosk_commands.cpp vosk_api_wrapper.cpp VoskRecognizer.cpp VADWrapperWebRTC.cpp VADWrapperSilero.cpp SileroVadIterator.cpp  AudioLogger.cpp RecognizerBase.cpp HunspellPostProc.cpp CustomPostProc.cpp \
ResamplerLibResample_48_16.cpp ResamplerLibResample_8_16.cpp ResamplerWebRTC_48_16.cpp ResamplerWebRTC_8_16.cpp RecIKTSImpl.cpp RepetitionRemover.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell -licuio -licuuc -lonnxruntime -lresample -L/onnxruntime-linux-x64-1.12.1/lib/
# -I/boost_1_76_0/ 

RUN mkdir -p /logs/ /model/

############################################
# Vosk server startup script
############################################

COPY vosk_server_recikts/startme.sh /
RUN chmod 755 startme.sh

CMD ["/startme.sh"]

# CMD ["./asr_server", "0.0.0.0" ,"2700", "5" ,"/opt/vosk-model-en/model"]
