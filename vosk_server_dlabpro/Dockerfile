FROM debian:bullseye-slim
MAINTAINER Daniel Sobe <daniel.sobe@sorben.com>

# normal call
# docker build -t vosk_server_dlabpro .

# rebuild from scratch
# docker build -t vosk_server_dlabpro . --no-cache

RUN apt update

RUN apt install -y g++ wget git make nano 

###################################
# Build WebRTC dependency
###################################

RUN git clone https://github.com/ZalozbaDev/webrtc-audio-processing.git webrtc-audio-processing
RUN cd webrtc-audio-processing && git checkout 6e37f37c4ea8790760b4c55d9ce9024a7e7bf260

RUN apt install -y meson libabsl-dev

RUN cd webrtc-audio-processing && meson . build -Dprefix=$PWD/install && ninja -C build

###################################
# Build Respeaker USB VAD dependency
###################################

RUN git clone https://github.com/ZalozbaDev/usb_4_mic_array.git usb_4_mic_array
RUN cd usb_4_mic_array && git checkout ce007685d4a31ed919bafc4dfe77c28e46bcd9a8

RUN apt install -y libusb-1.0-0 libusb-1.0-0-dev

RUN cd usb_4_mic_array/cpp/ && chmod 755 build_static.sh && ./build_static.sh

###################################
# Build vosk server wrapper dependency
###################################

RUN apt install -y libreadline-dev portaudio19-dev

RUN touch i.txt

RUN git clone https://github.com/ZalozbaDev/dLabPro_vosk_api_wrapper.git dLabPro_vosk_api_wrapper

# get boost source code for some boost header files 
RUN cd dLabPro_vosk_api_wrapper \
  && wget -q https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz \
  && tar xzf boost_1_76_0.tar.gz

# need to have the dlabpro recognizer header file available
RUN git clone https://github.com/ZalozbaDev/dLabPro.git dLabPro_vosk_api
RUN cd dLabPro_vosk_api && git checkout development_vosk_api

RUN cd dLabPro_vosk_api_wrapper && chmod 755 ./build_shared.sh && ./build_shared.sh

###################################
# Build recognition software
###################################

# seems running dcg is also a bad idea, so better not build it in the first place
# RUN cd dLabPro && GCC=clang GPP=clang++ make CLEANALL && GCC=clang GPP=clang++ make -C programs/dcg RELEASE && PATH=$PATH:$(pwd)/bin.release/ GCC=clang GPP=clang++ make -C programs/recognizer RELEASE
RUN cd dLabPro_vosk_api && make -j 4 -C programs/recognizer RELEASE

########################################################
# Setup locale to properly support sorbian diacritics
########################################################

RUN apt-get install -y locales

RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    locale-gen

ENV LC_ALL en_US.UTF-8 
ENV LANG en_US.UTF-8  
ENV LANGUAGE en_US:en     

COPY recognizer.cfg /

RUN mkdir /uasr-data/

COPY startme.sh /
RUN chmod 755 startme.sh

CMD ["/startme.sh"]

# run container manually (w/o compose)
## docker run --mount type=bind,source="$(pwd)"/uasr-data,target=/uasr-data/ -p 2700:2700 -it vosk_server_dlabpro /bin/bash
## LD_LIBRARY_PATH=/dLabPro_vosk_api_wrapper/ /dLabPro_vosk_api/bin.release/recognizer 0.0.0.0 2700 1 model_path

# copy audio logfiles
## rm uasr-data/*.raw
## docker exec -it CONTAINERID /bin/bash
## cp /vad_logs/* /uasr-data/

# CMD ["./asr_server", "0.0.0.0" ,"2700", "5" ,"/opt/vosk-model-en/model"]
