
#include <ResamplerLibResample_8_16.h>

#include <iostream>
#include <vector>

#include <libresample.h>

#include <cassert>

#include <MuLawDecoder.h>

//////////////////////////////////////////////
ResamplerLibResample_8_16::ResamplerLibResample_8_16()
{
	// params are adjusted for the 8-->16 case
	// minFactor = 1 (no downsampling required) 
	// maxFactor = 16000 / 8000 == 2 so 2.2 is OK
	resampleInst = resample_open(1, 1, 2.2);
    if (!resampleInst) {
        std::cerr << "Failed to initialize libresample." << std::endl;
        assert(false);
    }
}

//////////////////////////////////////////////
bool ResamplerLibResample_8_16::resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) 
{
	int16_t intermediatePCM8[sourceFrameLenSamples];
	int expTargetFramelen = sourceFrameLenSamples * 2;
	
	// 256 samples in 8 bit, 256 samples out 16 bit 
	size_t converted = MuLawDecoder::Convert(
		(uint8_t*) source,
		sourceFrameLenSamples,
		intermediatePCM8,
		sourceFrameLenSamples
    );
	
    assert(converted == sourceFrameLenSamples);
    
	// Convert source buffer to float
	std::vector<float> source_float(sourceFrameLenSamples);
	for (int i = 0; i < sourceFrameLenSamples; i++) 
	{
		   source_float[i] = ((float) intermediatePCM8[i]) / 32768.0f;
	}
	
	std::vector<float> target_float(expTargetFramelen);
	
	int input_used = sourceFrameLenSamples;
	
	int output_generated = resample_process(
		   resampleInst,
		   (2.0 / 1.0),
		   source_float.data(),
		   sourceFrameLenSamples,
		   1, // last buffer
		   &input_used,
		   target_float.data(),
		   expTargetFramelen
		   );
	
	// we expect that all samples are consumed and expected number of samples generated
	assert(input_used       == sourceFrameLenSamples);
	assert(output_generated == expTargetFramelen);

	for (int i = 0; i < output_generated; ++i) {
		   if (target_float[i] <= -1.0f)
		   {
		   	   target[i] = -32767;
		   }
		   else if (target_float[i] >= 1.0f)
		   {
		   	   target[i] = 32767;
		   }
		   else
		   {
		   	   target[i] = target_float[i] * 32768;
		   }
	}
	
	return true;
}

//////////////////////////////////////////////
ResamplerLibResample_8_16::~ResamplerLibResample_8_16()
{
	resample_close(resampleInst);
}
