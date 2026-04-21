
#include <ResamplerWebRTC_8_16.h>

#include <cassert>

#include <MuLawDecoder.h>


//////////////////////////////////////////////
ResamplerWebRTC_8_16::ResamplerWebRTC_8_16()
{
	WebRtcSpl_ResetResample48khzTo16khz(&m_resamplestate_48_to_16);
	WebRtcSpl_ResetResample8khzTo48khz(&m_resamplestate_8_to_48);
}

//////////////////////////////////////////////
bool ResamplerWebRTC_8_16::resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) 
{
	int16_t intermediatePCM8[sourceFrameLenSamples];
	
	int16_t intermediatePCM48[fixedBufferSize48khz];
	
	// WebRTC resampler can only cope with fixed buffer sizes
	assert(sourceFrameLenSamples == fixedBufferSize8khz);

	// 80 samples in 8 bit, 80 samples out 16 bit 
	size_t converted = MuLawDecoder::Convert(
		(uint8_t*) source,
		sourceFrameLenSamples,
		intermediatePCM8,
		sourceFrameLenSamples
    );
	
    assert(converted == sourceFrameLenSamples);
	
	// upsample 8 to 48 first (80 samples in 16bit, 480 samples out 16bit)
	WebRtcSpl_Resample8khzTo48khz(intermediatePCM8, intermediatePCM48, &m_resamplestate_8_to_48, tmp);
	
	// now resample from 48 kHz to 16 kHz (480 samples in 16bit, 160 samples out 16bit)
	WebRtcSpl_Resample48khzTo16khz(intermediatePCM48, target, &m_resamplestate_48_to_16, tmp);
	
	return true;
}

//////////////////////////////////////////////
ResamplerWebRTC_8_16::~ResamplerWebRTC_8_16()
{
	
}