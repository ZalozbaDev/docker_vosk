#ifndef RESAMPLER_WEBRTC_48_16_H
#define RESAMPLER_WEBRTC_48_16_H

#include <Resampler.h>

class ResamplerWebRTC_48_16 : public Resampler
{
public:
	ResamplerWebRTC_48_16();
	virtual ~Resampler();
	
	virtual int getSourceRateHz(void) override;
	virtual int getTargetRateHz(void) override;
	
	virtual bool resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) override;
}


#endif
