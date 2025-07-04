# Container improvements

* wav2vec2 and LM outside container

# Performance improvements

Jeli chceš ty tón wukon polěpšić bych Ći radźił tón wav2vec model optiměrować. Tón ngram model je tajki małki, tón trjeba lědma resursy.

* binary LM snano pomha, ale snano nic pře wjele

Jeli chceš ty tón połnu performance měć, by radźomne było zo spytaš raz torch.compile + flash attention abo Tensor RT + onnx. Ta standardna huggingface implementacija njeje na maksimalnu performance wusměrjena.

* torch.compile + flash attention
* Tensor RT + onnx

