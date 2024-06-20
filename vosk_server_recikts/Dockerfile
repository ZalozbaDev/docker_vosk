FROM debian:bullseye-slim
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
RUN cd webrtc-audio-processing && git checkout 6e37f37c4ea8790760b4c55d9ce9024a7e7bf260

RUN apt install -y meson libabsl-dev

RUN cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

############################################
# Build vosk server wrapper and recognizer
############################################

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

RUN apt install -y libhunspell-dev libicu-dev

COPY common/AudioLogger.cpp  common/AudioLogger.h  common/RecognitionResult.h  common/VADFrame.h  \
common/VADWrapper.cpp  common/VADWrapper.h  common/vosk_api_wrapper.cpp \
common/HunspellPostProc.h common/HunspellPostProc.cpp common/CustomPostProc.cpp \
common/CustomPostProc.h /
COPY vosk_server_recikts/recikts.h vosk_server_recikts/VoskRecognizer.cpp vosk_server_recikts/VoskRecognizer.h /

RUN mkdir -p /data/

RUN g++ -Wall -Wno-write-strings -std=c++17 -O3 -fPIC -o recikts_server -I/boost_1_76_0/ -I. \
-I/webrtc-audio-processing/webrtc/ -DPREFIX=\"/data/\" \
asr_server.cpp vosk_api_wrapper.cpp VoskRecognizer.cpp VADWrapper.cpp AudioLogger.cpp HunspellPostProc.cpp CustomPostProc.cpp \
webrtc-audio-processing/build/webrtc/common_audio/libcommon_audio.a \
-ldl -lpthread -lhunspell -licuio -licuuc

RUN mkdir -p /logs/

############################################
# Vosk server startup script
############################################

RUN apt install -y gdb

COPY vosk_server_recikts/startme.sh /
RUN chmod 755 startme.sh

CMD ["/startme.sh"]

# CMD ["./asr_server", "0.0.0.0" ,"2700", "5" ,"/opt/vosk-model-en/model"]
