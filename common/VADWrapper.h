
#ifndef VAD_WRAPPER_H
#define VAD_WRAPPER_H

#include <stdint.h>

#include <memory>
#include <cstddef>
#include <ctime>

#include <VADFrame.h>

enum VADWrapperState {IDLE, BUFFERING, POSTBUF};

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
	
    virtual const int getRequiredFrameLength(void) const = 0;
    virtual const int getFrameTimeMs(void) const = 0;
	virtual int process(int samplingFrequency, 
		        const int16_t* audio_frame, 
		        size_t frame_length, 
		        std::uint64_t frameCtr, 
		        std::chrono::time_point<std::chrono::system_clock> frameTime) = 0;
	virtual bool analyze(bool hintShortAudio = false) = 0;
	virtual unsigned int getAvailableChunks(void) = 0;
	virtual VADWrapperState getUtteranceStatus(void) = 0;
	virtual std::unique_ptr<VADFrame> getNextChunk(void) = 0;
	virtual int64_t getUtteranceStart(void) = 0;
	virtual int64_t getUtteranceStartMs(void) = 0;
	virtual int64_t getUtteranceStop(void) = 0;
	virtual int64_t getUtteranceStopMs(void) = 0;
	
	virtual uint64_t getUtteranceStartFrameCtr(void) = 0;
	virtual uint64_t getUtteranceStopFrameCtr(void) = 0;
	
};

#endif
