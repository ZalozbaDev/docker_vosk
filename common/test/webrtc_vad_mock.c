
#include "webrtc_vad_mock.h"

#include "assert.h"

#include <stdio.h>

struct WebRtcVadInst
{
	int16_t sample_ctr;
};

VadInst dummy;

VadInst* WebRtcVad_Create(void)
{
	printf("WebRtcVad_Create()\n");
	dummy.sample_ctr = 0;
	return &dummy;
}

void WebRtcVad_Free(VadInst* handle)
{
	printf("WebRtcVad_Free()\n");
	dummy.sample_ctr = 0;
}

int WebRtcVad_Init(VadInst* handle)
{
	return 0;	
}

int WebRtcVad_set_mode(VadInst* handle, int mode)
{
	return 0;
}

int WebRtcVad_Process(VadInst* handle,
                      int fs,
                      const int16_t* audio_frame,
                      size_t frame_length)
{
	printf("WebRtcVad_Process(frame_length=%ld)\n", frame_length);
	for (size_t index = 0; index < frame_length; index++)
	{
		if (audio_frame[index] != dummy.sample_ctr)
		{
			printf("Sample comparison error! Expected sample %d but read %d at index %ld.\n", dummy.sample_ctr, audio_frame[index], index);
			assert(false);
		}
		dummy.sample_ctr++;
	}
	return 0;	
}

int WebRtcVad_ValidRateAndFrameLength(int rate, size_t frame_length)
{
	return 0;	
}

void WebRtcVad_Mock_reset(VadInst* handle)
{
	printf("WebRtcVad_Mock_reset()\n");
	dummy.sample_ctr = 0;
}
