# Overall test strategy

See this document:

https://github.com/ZalozbaDev/language_modeling_boze_mse/blob/main/TEST_STRATEGY.md

# Test VAD algorithm implementation

## recikts tests

### testdata/0001_citanje.wav

```code
LD_LIBRARY_PATH=$RECIKTSTOOLING recikts_out/recikts_main ~/docker_compose_files_private/hetzner_serwer/webcaptioner-ng/data/merged_47_nnet_v3.cfg testdata/0001_citanje.wav testresults/
diff testdata/0001_citanje.srt testresults/subtitles.srt
```



TBD

