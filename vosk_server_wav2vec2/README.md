# docker setup

## build container


```code
docker build -t vosk_server_wav2vec2 --progress=plain .
```

## provide model and other data

clone the model from git:

```code
cd models
mkdir -p Korla
cd Korla
git lfs install
git clone https://huggingface.co/Korla/Wav2Vec2BertForCTC-hsb-0
cd ../../
```

(optional) clone the optimized ONNX model from git:

```code
cd models/Korla
git clone https://huggingface.co/Korla/onnx-models
mkdir -p ../onnx
cp onnx-models/wav2vec2.onnx* ../onnx/
cd ../../
```

(optional) provide and use a diferent LM

TBD: path of LM is fixed to "lm/5gram_correct.arpa"

(optional) provide hunspell directory

TBD: path is fixed to "spell/hsb.dic" resp. "spell/hsb.aff"

## run the container

use the provided compose file, copy to "docker-compose.yml"

copy "env.example" to ".env" and adjust settings

run the container

# TODO

## System improvements

* support more sample rates, not just 16 kHz

## Container improvements

* wav2vec2 and LM outside container

## Performance improvements

Jeli chceš ty tón wukon polěpšić bych Ći radźił tón wav2vec model optiměrować. Tón ngram model je tajki małki, tón trjeba lědma resursy.

* binary LM snano pomha, ale snano nic pře wjele

Jeli chceš ty tón połnu performance měć, by radźomne było zo spytaš raz torch.compile + flash attention abo Tensor RT + onnx. Ta standardna huggingface implementacija njeje na maksimalnu performance wusměrjena.

* torch.compile + flash attention
* Tensor RT + onnx

