#ifndef RESAMPLER_WEBRTC_48_16_H
#define RESAMPLER_WEBRTC_48_16_H

#include <Resampler.h>

#ifndef SIGNAL_PROCESSING_MOCK
extern "C" {
#include "common_audio/signal_processing/include/signal_processing_library.h"
}
#else
#include "signal_processing_mock.h"
#endif

class ResamplerWebRTC_48_16 : public Resampler
{
public:
	ResamplerWebRTC_48_16();
	virtual ~ResamplerWebRTC_48_16();
	
	virtual int getSourceRateHz(void) override { return sourceRateHz; }
	virtual int getTargetRateHz(void) override { return targetRateHz; }
	
	virtual bool resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) override;
	
private:
	const int sourceRateHz = 48000;
	const int targetRateHz = 16000;
	
	const static unsigned int fixedBufferSize48khz = 480;
	
	WebRtcSpl_State48khzTo16khz m_resamplestate_48_to_16;
	
	// according to webrtc-audio-processing/webrtc/common_audio/resampler/resampler.cc it looks
	// like tmpbuf must be 16 entries longer than the 48kHz buffer, so 256 is some overhead but save
	int32_t tmp[fixedBufferSize48khz + 256] = { 0 };
};

#endif
