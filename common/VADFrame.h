
#include <cstddef>
#include <ctime>
#include <cstdint>

#include <chrono>

enum VADState {OFF, ACTIVE};

class VADFrame
{
public:
	short*    samples;
#ifdef VAD_FRAME_CONVERT_FLOAT	
	float*    fSamples;
#endif
	std::size_t m_numberSamples;
	VADState state;
	std::uint64_t currFrameCtr;
	std::chrono::time_point<std::chrono::system_clock> currFrameTime;

	VADFrame(std::size_t numberSamples)
	{
		m_numberSamples = numberSamples;
		samples = new short[numberSamples];
#ifdef VAD_FRAME_CONVERT_FLOAT	
		fSamples = new float[numberSamples]		
#endif
	}
	
	~VADFrame(void)
	{
#ifdef VAD_FRAME_CONVERT_FLOAT	
		delete[] fSamples;
#endif
		delete[] samples;
	}
};
