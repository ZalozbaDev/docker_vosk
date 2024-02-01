#ifndef WEBRTC_VAD_MOCK_H_
#define WEBRTC_VAD_MOCK_H_

#include <stddef.h>
#include <stdint.h>

typedef struct WebRtcVadInst VadInst;

#ifdef __cplusplus
extern "C" {
#endif

VadInst* WebRtcVad_Create(void);

void WebRtcVad_Free(VadInst* handle);

int WebRtcVad_Init(VadInst* handle);

int WebRtcVad_set_mode(VadInst* handle, int mode);

int WebRtcVad_Process(VadInst* handle,
                      int fs,
                      const int16_t* audio_frame,
                      size_t frame_length);

int WebRtcVad_ValidRateAndFrameLength(int rate, size_t frame_length);

void WebRtcVad_Mock_reset(VadInst* handle);

#ifdef __cplusplus
}
#endif

#endif  // WEBRTC_VAD_MOCK_H_
