
#include "signal_processing_mock.h"

#include "assert.h"

#include <stdio.h>

void WebRtcSpl_ResetResample48khzTo16khz(WebRtcSpl_State48khzTo16khz* state)
{
	
}

// downsampling without filtering
void WebRtcSpl_Resample48khzTo16khz(const int16_t* in,
                                    int16_t* out,
                                    WebRtcSpl_State48khzTo16khz* state,
                                    int32_t* tmpmem)
{
	for (int i = 0; i < 480; i += 3)
	{
		out[i / 3] = in[i];
	}
}

