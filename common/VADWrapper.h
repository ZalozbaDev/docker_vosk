
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

enum VADWrapperState {IDLE, BUFFERING, POSTBUF};

//////////////////////////////////////////////
class VADWrapper
{
public:
	static const unsigned int nrVADSamples = 160;
	
	static const unsigned int vadMaxNrToggles = 10;
	
	VADWrapper(int aggressiveness, 
		       size_t frequencyHz, 
		       unsigned int audioPreBufferFrames  = 15,
	           unsigned int audioPostBufferFrames = 15, 
	           unsigned int vadHystheresisFramesOn = 5,
	           unsigned int vadHystheresisFramesOff = 5);
	~VADWrapper(void);
	int process(int samplingFrequency, 
		        const int16_t* audio_frame, 
		        size_t frame_length, 
		        std::uint64_t frameCtr, 
		        std::chrono::time_point<std::chrono::system_clock> frameTime);
	bool analyze(bool hintShortAudio = false);
	unsigned int getAvailableChunks(void);
	VADWrapperState getUtteranceStatus(void) { return state; }
	std::unique_ptr<VADFrame> getNextChunk(void);
	int64_t getUtteranceStart(void)   { return uStartTime;   }
	int64_t getUtteranceStartMs(void) { return uStartTimeMs; }
	int64_t getUtteranceStop(void)    { return uStopTime;    }
	int64_t getUtteranceStopMs(void)  { return uStopTimeMs;  }
	
	uint64_t getUtteranceStartFrameCtr(void)  { return frameCtrStart;  }
	uint64_t getUtteranceStopFrameCtr(void)   { return frameCtrStop;  }
	
private:
	VadInst* rtcVadInst;
	
	std::deque<std::unique_ptr<VADFrame>> chunks;
	
	const unsigned int m_audioPreBufferFrames;
	const unsigned int m_audioPostBufferFrames;
	
	const unsigned int m_vadHystheresisFramesOn;
	const unsigned int m_vadHystheresisFramesOff;
	
	VADWrapperState state;
	
	unsigned int m_analyzeStopOffset;
	
	// offset in the chunks queue until the end of the current utterance
	// chunks until here are removed when read (and offset decreased)
	unsigned int m_unbufferedStopChunksOffset;
	
	// counter for frames that must be kept in queue when being read out
	// this cannot be an offset because the frames might not be available yet
	unsigned int m_bufferedStopChunksCountDown;
	
	// for live recognition use
	int64_t uStartTime;
	int64_t uStartTimeMs;
	int64_t uStopTime;
	int64_t uStopTimeMs;

	// for cmdline usage
	std::uint64_t frameCtrStart;
	std::uint64_t frameCtrStop;
	
	bool findUtteranceStart(void);
	void findUtteranceStop(bool hintShortAudio);

#ifdef TEST_VADWRAPPER

public:
	VadInst* getRtcVadInst(void) { return rtcVadInst; }
#endif	
};

#endif
