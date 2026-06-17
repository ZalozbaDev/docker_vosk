
#ifndef VAD_WRAPPER_SILERO_H
#define VAD_WRAPPER_SILERO_H

#include <stdint.h>

#include <deque>
#include <memory>
#include <cstddef>
#include <ctime>

#include <VADWrapper.h>

#include <SileroVadIterator.h>

//////////////////////////////////////////////
class VADWrapperSilero : public VADWrapper
{
public:
	VADWrapperSilero(size_t frequencyHz, const std::string model_path,
				unsigned int audioPreBufferFrames  = 3,
	           unsigned int audioPostBufferFrames = 3, 
	           unsigned int vadHystheresisFramesOn = 1,
	           unsigned int vadHystheresisFramesOff = 2);
	virtual ~VADWrapperSilero(void);
	virtual int process(int samplingFrequency, 
		        const int16_t* audio_frame, 
		        size_t frame_length, 
		        std::uint64_t frameCtr, 
		        std::chrono::time_point<std::chrono::system_clock> frameTime) override;
	virtual bool analyze(bool hintShortAudio = false) override;
	virtual unsigned int getAvailableChunks(void) override;
	virtual VADWrapperState getUtteranceStatus(void) override { return state; }
	virtual std::unique_ptr<VADFrame> getNextChunk(void) override;
	virtual std::unique_ptr<VADFrameTiming> getUtteranceStart(void) override;
	virtual std::unique_ptr<VADFrameTiming> getUtteranceStop(void) override;
	virtual std::unique_ptr<VADFrameTiming> getUtteranceCurr(void) override;
	
	virtual int getRequiredFrameLength(void) const override { return nrVADSamples; }

	virtual int getFrameTimeMs(void) const override { return 32; }

private:
	
	// silero default
	static const unsigned int nrVADSamples = 512;
	
	static const unsigned int vadMaxNrToggles = 10;
	
	silero::VadIterator* sileroVadInst;
	
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
	
	// for fragmented PCM buffer
	std::uint64_t frameCtrCurr;
	std::chrono::time_point<std::chrono::system_clock> timeStampCurr;
	
	bool findUtteranceStart(void);
	void findUtteranceStop(bool hintShortAudio);

};

#endif
