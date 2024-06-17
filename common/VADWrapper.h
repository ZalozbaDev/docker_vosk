
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
	int process(int samplingFrequency, const int16_t* audio_frame, size_t frame_length);
	bool analyze(void);
	unsigned int getAvailableChunks(void);
	VADWrapperState getUtteranceStatus(void) { return state; }
	std::unique_ptr<VADFrame<nrVADSamples>> getNextChunk(void);
	time_t getUtteranceStart(void) { return startTime; }
	time_t getUtteranceStop(void) { return stopTime; }
	
private:
	VadInst* rtcVadInst;
	
	std::deque<std::unique_ptr<VADFrame<nrVADSamples>>> chunks;
	
	short       leftOverSamples[nrVADSamples];
	std::size_t leftOverSampleSize;
	
	static const unsigned int prebufVal  = 5;
	static const unsigned int postbufVal = 5;
	
	VADWrapperState state;
	int utteranceCurr;
	
	time_t startTime;
	time_t stopTime;
	
	bool findUtteranceStart(void);
	void findUtteranceStop(void);

#ifdef TEST_VADWRAPPER

public:
	std::size_t getLeftOverSampleSize(void) { return leftOverSampleSize; }
	VadInst* getRtcVadInst(void) { return rtcVadInst; }
#endif	
};

#endif
