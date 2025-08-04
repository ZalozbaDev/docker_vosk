#ifndef RESAMPLER_H
#define RESAMPLER_H

class Resampler
{
public:
	Resampler()            = default;
	virtual ~Resampler()   = default;
	
	virtual int getSourceRateHz(void) = 0;
	virtual int getTargetRateHz(void) = 0;
	
	virtual bool resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) = 0;
}


#endif
