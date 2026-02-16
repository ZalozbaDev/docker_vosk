
#include <ResamplerWebRTC_8_16.h>

#include <cassert>

//////////////////////////////////////////////
ResamplerWebRTC_8_16::ResamplerWebRTC_8_16()
{
	WebRtcSpl_ResetResample48khzTo16khz(&m_resamplestate_48_to_16);
	WebRtcSpl_ResetResample8khzTo48khz(&m_resamplestate_8_to_48);
}

//////////////////////////////////////////////
bool ResamplerWebRTC_8_16::resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) 
{
	int16_t intermediate[fixedBufferSize48khz];
	
	// WebRTC resampler can only cope with fixed buffer sizes
	assert(sourceFrameLenSamples == fixedBufferSize8khz);
	
	// upsample 8 to 48 first
	WebRtcSpl_Resample8khzTo48khz(source, intermediate, &m_resamplestate_8_to_48, tmp);
	
	// now resample from 48 kHz to 16 kHz
	WebRtcSpl_Resample48khzTo16khz(intermediate, target, &m_resamplestate_48_to_16, tmp);
	
	return true;
}

//////////////////////////////////////////////
ResamplerWebRTC_8_16::~ResamplerWebRTC_48_16()
{
	
}