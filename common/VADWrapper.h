
#ifndef VAD_WRAPPER_H
#define VAD_WRAPPER_H

#include <stdint.h>

#include <deque>
#include <memory>
#include <cstddef>
#include <ctime>

#include <VADFrame.h>

#ifndef WEBRTC_VAD_MOCK
extern "C" {
#include "webrtc-audio-processing/webrtc/common_audio/vad/include/webrtc_vad.h"
}
#else
#include "webrtc_vad_mock.h"
#endif

enum VADWrapperState {IDLE, INCOMPLETE, COMPLETE};

//////////////////////////////////////////////
class VADWrapper
{
public:
	static const unsigned int nrVADSamples = 160;
	
	VADWrapper(int aggressiveness, size_t frequencyHz, unsigned int prebufVal = 5,
	           unsigned int postbufValShort = 10, unsigned int postbufValLong = 5,
	           unsigned int uttTriggerVal = 5);
	~VADWrapper(void);
	int process(int samplingFrequency, const int16_t* audio_frame, size_t frame_length, std::uint64_t frameCtr, std::chrono::time_point<std::chrono::system_clock> frameTime);
	bool analyze(bool hintShortAudio = false);
	unsigned int getAvailableChunks(void);
	VADWrapperState getUtteranceStatus(void) { return state; }
	std::unique_ptr<VADFrame<nrVADSamples>> getNextChunk(void);
	int64_t getUtteranceStart(void)   { return uStartTime;   }
	int64_t getUtteranceStartMs(void) { return uStartTimeMs; }
	int64_t getUtteranceStop(void)    { return uStopTime;    }
	int64_t getUtteranceStopMs(void)  { return uStopTimeMs;  }
	
	uint64_t getUtteranceStartFrameCtr(void)  { return frameCtrStart;  }
	uint64_t getUtteranceStopFrameCtr(void)   { return frameCtrStop;  }
	
private:
	VadInst* rtcVadInst;
	
	std::deque<std::unique_ptr<VADFrame<nrVADSamples>>> chunks;
	
	short       leftOverSamples[nrVADSamples];
	std::size_t leftOverSampleSize;
	
	const unsigned int m_prebufVal;
	
	const unsigned int m_postbufValShort;
	const unsigned int m_postbufValLong;
	
	const unsigned int m_uttTriggerVal;
	
	unsigned int prebufCtrStart;
	unsigned int prebufCtrToggle;
	unsigned int postBufCtrStop;
	
	VADWrapperState state;
	int utteranceCurr;
	
	int64_t uStartTime;
	int64_t uStartTimeMs;
	int64_t uStopTime;
	int64_t uStopTimeMs;
	
	std::uint64_t frameCtrStart;
	std::uint64_t frameCtrStop;
	
	
	bool findUtteranceStart(void);
	void findUtteranceStop(bool hintShortAudio);

#ifdef TEST_VADWRAPPER

public:
	std::size_t getLeftOverSampleSize(void) { return leftOverSampleSize; }
	VadInst* getRtcVadInst(void) { return rtcVadInst; }
#endif	
};

#endif
