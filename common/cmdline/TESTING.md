# Overall test strategy

See this document:

https://github.com/ZalozbaDev/language_modeling_boze_mse/blob/main/TEST_STRATEGY.md

# Test VAD algorithm implementation

## recikts tests

### testdata/0001_citanje.wav

```code
LD_LIBRARY_PATH=$RECIKTSTOOLING recikts_out/recikts_main ~/docker_compose_files_private/hetzner_serwer/webcaptioner-ng/data/merged_47_nnet_v3.cfg testdata/0001_citanje.wav testresults/ 3
diff testdata/0001_citanje_vadaggr_3.srt testresults/subtitles.srt
```

```code
LD_LIBRARY_PATH=$RECIKTSTOOLING recikts_out/recikts_main ~/docker_compose_files_private/hetzner_serwer/webcaptioner-ng/data/merged_47_nnet_v3.cfg testdata/0001_citanje.wav testresults/ 2
diff testdata/0001_citanje_vadaggr_2.srt testresults/subtitles.srt
```

```code
LD_LIBRARY_PATH=$RECIKTSTOOLING recikts_out/recikts_main ~/docker_compose_files_private/hetzner_serwer/webcaptioner-ng/data/merged_47_nnet_v3.cfg testdata/0001_citanje.wav testresults/ 1
diff testdata/0001_citanje_vadaggr_1.srt testresults/subtitles.srt
```

### testdata/0002_poswjecenje.wav

```code
LD_LIBRARY_PATH=$RECIKTSTOOLING recikts_out/recikts_main ~/docker_compose_files_private/hetzner_serwer/webcaptioner-ng/data/merged_47_nnet_v3.cfg testdata/0002_poswjecenje.wav testresults/ 3
diff testdata/0002_poswjecenje_vadaggr_3.srt testresults/subtitles.srt
```

```code
LD_LIBRARY_PATH=$RECIKTSTOOLING recikts_out/recikts_main ~/docker_compose_files_private/hetzner_serwer/webcaptioner-ng/data/merged_47_nnet_v3.cfg testdata/0002_poswjecenje.wav testresults/ 2
diff testdata/0002_poswjecenje_vadaggr_2.srt testresults/subtitles.srt
```

```code
LD_LIBRARY_PATH=$RECIKTSTOOLING recikts_out/recikts_main ~/docker_compose_files_private/hetzner_serwer/webcaptioner-ng/data/merged_47_nnet_v3.cfg testdata/0002_poswjecenje.wav testresults/ 1
diff testdata/0002_poswjecenje_vadaggr_1.srt testresults/subtitles.srt
```

## whisper tests

```code
LD_LIBRARY_PATH=./whisper_out/:./onnxruntime-linux-x64-1.12.1/lib/ ./whisper_out/whisper_main ~/whisper_models/Korla/whisper_large_v3_turbo_hsb/ggml-model.bin testdata/0001_citanje.wav testresults/ 2
diff testdata/0001_citanje_vadaggr_2_whisper.srt testresults/subtitles.srt
```

## VAD test

```code
LD_LIBRARY_PATH=./whisper_out/:./onnxruntime-linux-x64-1.12.1/lib/ ./whisper_out/whisper_main ~/whisper_models/Korla/whisper_large_v3_turbo_hsb/ggml-model.bin testdata/0001_citanje.wav testresults/ 2 auto -1 False Silero 
diff testdata/0001_citanje_vadaggr_2_whisper.srt testresults/subtitles.srt
```
