
#include <ResamplerLibResample_48_16.h>

#include <iostream>
#include <vector>

#include <libresample.h>

#include <cassert>

//////////////////////////////////////////////
ResamplerLibResample_48_16::ResamplerLibResample_48_16()
{
	// params are adjusted for the 48-->16 case
	resampleInst = resample_open(1, 0.3, 1);
    if (!resampleInst) {
        std::cerr << "Failed to initialize libresample." << std::endl;
        assert(false);
    }
}

//////////////////////////////////////////////
bool ResamplerLibResample_48_16::resample(const int16_t* source, int16_t* target, const int sourceFrameLenSamples) 
{
	int expTargetFramelen = sourceFrameLenSamples / 3;
	
	// Convert source buffer to float
	std::vector<float> source_float(sourceFrameLenSamples);
	for (int i = 0; i < sourceFrameLenSamples; i++) 
	{
		   source_float[i] = ((float) source[i]) / 32768.0f;
	}
	
	std::vector<float> target_float(expTargetFramelen);
	
	int input_used = sourceFrameLenSamples;
	
	int output_generated = resample_process(
		   resampleInst,
		   (1.0 / 3.0),
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
		   	   target[i] = 32768;
		   }
		   else
		   {
		   	   target[i] = target_float[i] * 32768;
		   }
	}
	
	return true;
}

//////////////////////////////////////////////
ResamplerLibResample_48_16::~ResamplerLibResample_48_16()
{
	resample_close(resampleInst);
}
