
#include <ResamplerWebRTC_48_16.h>

#include <cassert>

//////////////////////////////////////////////
ResamplerWebRTC_48_16::ResamplerWebRTC_48_16()
{
	WebRtcSpl_ResetResample48khzTo16khz(&m_resamplestate_48_to_16);
}

//////////////////////////////////////////////
bool ResamplerWebRTC_48_16::resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) 
{
	// WebRTC resampler can only cope with fixed buffer sizes
	assert(sourceFrameLenSamples == fixedBufferSize48khz);
	
	WebRtcSpl_Resample48khzTo16khz(source, target, &m_resamplestate_48_to_16, tmp);
	
	return true;
}

//////////////////////////////////////////////
ResamplerWebRTC_48_16::~ResamplerWebRTC_48_16()
{
	
}