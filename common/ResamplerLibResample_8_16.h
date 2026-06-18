#ifndef RESAMPLER_LIBRESAMPLE_8_16_H
#define RESAMPLER_LIBRESAMPLE_8_16_H

#include <Resampler.h>

class ResamplerLibResample_8_16 : public Resampler
{
public:
	ResamplerLibResample_8_16();
	virtual ~ResamplerLibResample_8_16();
	
	virtual int getSourceRateHz(void) override { return sourceRateHz; }
	virtual int getTargetRateHz(void) override { return targetRateHz; }
	
	virtual bool resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) override;
	
private:
	const int sourceRateHz = 8000;
	const int targetRateHz = 16000;
	
	void* resampleInst;
};

#endif
