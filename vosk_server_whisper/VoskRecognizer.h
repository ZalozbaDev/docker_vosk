#ifndef VOSK_RECOGNIZER_H
#define VOSK_RECOGNIZER_H

#include "RecognizerBase.h"

#include <iostream>
#include <vector>

#include <queue>

#include <thread>
#include <mutex>
#include <condition_variable>

extern "C" {
#include "vosk_api.h"
}

#include <VADWrapper.h>
#include <Resampler.h>
#include <RecognitionResult.h>
#include <AudioLogger.h>

#include <HunspellPostProc.h>
#include <CustomPostProc.h>

#include "WhisperImpl.h"

//////////////////////////////////////////////
class VoskRecognizer:public RecognizerBase
{
public:
	VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness=2);
	virtual ~VoskRecognizer(void);
	
	virtual int getInstanceId(void)                                       override { return m_instanceId; }
	virtual int getModelInstanceId(void)                                  override { return m_modelInstanceId; }
	virtual float getSampleRate(void)                                     override { return m_inputSampleRate; }
	virtual void setDetailedResult(bool detailsOn)                        override;
	virtual int acceptWaveform(const char *data, int length)              override;
	virtual bool getRecognizerBusy(bool audioQueueOnly = false)           override;
	virtual const char* getPartialResult(void)                            override;
	virtual const char* getFinalResult(void)                              override;
	virtual bool getPartialStatus(void)                                   override;
	virtual std::unique_ptr<RecognizedUtterance> getFinalResultData(void) override;
	virtual int getFrameResolution(void)                                  override;

	// TBD move to base?
	void setTimeStamp(int64_t seconds, int64_t uSeconds);
	void resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood);
	
private:
	static const ssize_t m_processingSampleRate = 16000;
	
	static int voskRecognizerInstanceId;

	int m_instanceId;
	int m_modelInstanceId;
	float m_inputSampleRate;
	bool m_libraryLoaded;
	VoskRecognizerState m_recoState;
	uint64_t m_vadFrameCounter;
	
	std::thread *recoWorkerThread;
	bool threadRunning;
	std::deque<std::unique_ptr<AudioPacket>> audioPackets;
	std::mutex audioPacketMutex;
	std::condition_variable audioPacketNotify;
	void workerThreadFunc(void);

	std::string m_configPath;

	std::vector<float> pcmf32;
	bool pcmBufferFragmented;
	std::unique_ptr<VADFrameTiming> currFragmentStartTime;
		
	VADWrapper *vad;
	Resampler  *resample;
	
	char* leftOverData;
	int leftOverDataLen = 0;
	
	WhisperImpl *whisperImpl;

	void runTokensToWords(void);

	std::chrono::time_point<std::chrono::system_clock> clientTimeStamp;
	
	std::vector<std::unique_ptr<RecognizedToken>>    tokens;
	std::mutex tokenMutex;
	
	std::vector<std::unique_ptr<RecognizedWord>>     words;
	std::mutex wordMutex;
	
	std::deque<std::unique_ptr<RecognizedUtterance>> utterances;
	std::mutex utteranceMutex;
	
	// to avoid early deletion of string objects, use preallocated memory for the most recent string
	char partialResultBuffer[1000];
	char finalResultBuffer[100000];
	bool detailedResults;
	
	void promoteToFinalResult(std::unique_ptr<VADFrameTiming> currStart, std::unique_ptr<VADFrameTiming> currStop);
	void runWhisper(struct whisper_context* ctx);
	
	AudioLogger *audioLogger;
	
	HunspellPostProc *hpp;
	CustomPostProc *cpp;
};

#endif // VOSK_RECOGNIZER_H

