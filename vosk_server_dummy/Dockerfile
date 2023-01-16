FROM debian:bullseye-slim
MAINTAINER Daniel Sobe <daniel.sobe@sorben.com>

# normal call
# docker build -t vosk_server_dummy .

# rebuild from scratch
# docker build -t vosk_server_dummy . --no-cache

RUN apt update

RUN apt install -y g++ wget

# get boost source code for some boost header files 
RUN cd /opt \
  && wget -q https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz \
  && tar xzf boost_1_76_0.tar.gz

RUN apt install -y git  
  
# get the VOSK API
RUN git clone https://github.com/alphacep/vosk-api /opt/vosk-api

# get the VOSK server code
RUN git clone https://github.com/alphacep/vosk-server /opt/vosk-server

# add our own sources
COPY dummy.c /

# compile the CPP server  
RUN cd /opt/vosk-server/websocket-cpp \
    && g++ -std=c++17 -O3 -I/opt/boost_1_76_0 -I/opt/vosk-api/src -o asr_server asr_server.cpp /dummy.c -L/opt/vosk-api/src -Wl,-rpath=/opt/vosk-api/src -lpthread -ldl 

EXPOSE 2700
WORKDIR /opt/vosk-server/websocket-cpp
ENV LD_LIBRARY_PATH /opt
ENV VOSK_SHOW_WORDS False
ENV VOSK_ALTERNATIVES 1
ENV VOSK_SAMPLE_RATE 8000

CMD ["./asr_server", "0.0.0.0" ,"2700", "5" ,"/opt/vosk-model-en/model"]
