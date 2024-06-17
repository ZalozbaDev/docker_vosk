#ifndef SIGNAL_PROCESSING_MOCK_H_
#define SIGNAL_PROCESSING_MOCK_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int32_t S_48_48[16];
  int32_t S_48_32[8];
  int32_t S_32_16[8];
} WebRtcSpl_State48khzTo16khz;

void WebRtcSpl_ResetResample48khzTo16khz(WebRtcSpl_State48khzTo16khz* state);

void WebRtcSpl_Resample48khzTo16khz(const int16_t* in,
                                    int16_t* out,
                                    WebRtcSpl_State48khzTo16khz* state,
                                    int32_t* tmpmem);

#ifdef __cplusplus
}
#endif

#endif  // SIGNAL_PROCESSING_MOCK_H_
