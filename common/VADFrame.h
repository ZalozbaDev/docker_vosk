
#include <cstddef>

enum VADState {OFF, ACTIVE};

template<std::size_t numberSamples>
class VADFrame
{
public:
	short    samples[numberSamples];
#ifdef VAD_FRAME_CONVERT_FLOAT	
	float    fSamples[numberSamples];
#endif	
	VADState state;
};
