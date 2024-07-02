
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
	
	VADWrapper(int aggressiveness, size_t frequencyHz);
	~VADWrapper(void);
	int process(int samplingFrequency, const int16_t* audio_frame, size_t frame_length, time_t startTime = 0);
	bool analyze(bool hintShortAudio = false);
	unsigned int getAvailableChunks(void);
	VADWrapperState getUtteranceStatus(void) { return state; }
	std::unique_ptr<VADFrame<nrVADSamples>> getNextChunk(void);
	int64_t getUtteranceStart(void)   { return uStartTime;   }
	int64_t getUtteranceStartMs(void) { return uStartTimeMs; }
	int64_t getUtteranceStop(void)    { return uStopTime;    }
	int64_t getUtteranceStopMs(void)  { return uStopTimeMs;  }
	
private:
	VadInst* rtcVadInst;
	
	std::deque<std::unique_ptr<VADFrame<nrVADSamples>>> chunks;
	
	short       leftOverSamples[nrVADSamples];
	std::size_t leftOverSampleSize;
	
	static const unsigned int prebufVal = 5;
	
	// collect more audio if the utterance is still short
	static const unsigned int postbufValShort = 10;
	static const unsigned int postbufValLong  = 5;
	
	unsigned int prebufCtrStart;
	unsigned int prebufCtrToggle;
	unsigned int postBufCtrStop;
	
	VADWrapperState state;
	int utteranceCurr;
	
	int64_t uStartTime;
	int64_t uStartTimeMs;
	int64_t uStopTime;
	int64_t uStopTimeMs;
	
	bool findUtteranceStart(void);
	void findUtteranceStop(bool hintShortAudio);

#ifdef TEST_VADWRAPPER

public:
	std::size_t getLeftOverSampleSize(void) { return leftOverSampleSize; }
	VadInst* getRtcVadInst(void) { return rtcVadInst; }
#endif	
};

#endif
