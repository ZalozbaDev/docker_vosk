
#ifndef VAD_WRAPPER_H
#define VAD_WRAPPER_H

#include <stdint.h>

#include <memory>
#include <cstddef>
#include <ctime>

#include <VADFrame.h>

enum VADWrapperState {IDLE, BUFFERING, POSTBUF};

class VADFrameTiming
{
public:
	VADFrameTiming() {
		valid = false;
	}
	
	bool valid;
	uint64_t frameCounter;
	int64_t  timeStampSeconds;
	int64_t  timeStampMilliSeconds;
};

//////////////////////////////////////////////
class VADWrapper
{
public:
	VADWrapper()              = default;
	virtual ~VADWrapper(void) = default;
	
	VADWrapper(const VADWrapper&)            = delete;          
    VADWrapper& operator=(const VADWrapper&) = delete; 
    VADWrapper(VADWrapper&&)                 = delete;               
    VADWrapper& operator=(VADWrapper&&)      = delete;    
	
    virtual int getRequiredFrameLength(void) const = 0;
    virtual int getFrameTimeMs(void) const = 0;
	virtual int process(int samplingFrequency, 
		        const int16_t* audio_frame, 
		        size_t frame_length, 
		        std::uint64_t frameCtr, 
		        std::chrono::time_point<std::chrono::system_clock> frameTime) = 0;
	virtual bool analyze(bool hintShortAudio = false) = 0;
	virtual unsigned int getAvailableChunks(void) = 0;
	virtual VADWrapperState getUtteranceStatus(void) = 0;
	virtual std::unique_ptr<VADFrame> getNextChunk(void) = 0;
	virtual std::unique_ptr<VADFrameTiming> getUtteranceStart(void) = 0;
	virtual std::unique_ptr<VADFrameTiming> getUtteranceStop(void) = 0;
	virtual std::unique_ptr<VADFrameTiming> getUtteranceCurr(void) = 0;
};

#endif
