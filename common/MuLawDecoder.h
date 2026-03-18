#ifndef MULAW_DECODER_H
#define MULAW_DECODER_H

#include <cstdint>
#include <cstddef>

class MuLawDecoder
{
public:
    // Converts μ-Law (8-bit) to 16-bit PCM.
    //
    // input        : pointer to μ-law bytes
    // inputLength  : number of input bytes
    // output       : caller-supplied buffer for PCM samples
    // outputLength : number of int16_t slots available in output
    //
    // Returns number of samples written to output.
    static size_t Convert(const uint8_t* input,
                          size_t inputLength,
                          int16_t* output,
                          size_t outputLength)
    {
        if (!input || !output)
            return 0;

        size_t samplesToProcess = (inputLength < outputLength)
                                  ? inputLength
                                  : outputLength;

        for (size_t i = 0; i < samplesToProcess; ++i)
        {
            output[i] = DecodeSample(input[i]);
        }

        return samplesToProcess;
    }

private:
	static int16_t DecodeSample(uint8_t muLawByte)
	{
		muLawByte = ~muLawByte;
	
		int sign     = muLawByte & 0x80;
		int exponent = (muLawByte >> 4) & 0x07;
		int mantissa =  muLawByte & 0x0F;
	
		int sample = ((mantissa | 0x10) << (exponent + 3)) - 132;
	
		return sign ? -sample : sample;
	}
};

#endif
