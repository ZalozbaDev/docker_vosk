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

#include "WhisperPool.h"

//////////////////////////////////////////////
class VoskRecognizer:public RecognizerBase
{
public:
	VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness=2);
	virtual ~VoskRecognizer(void);
	
	virtual int getInstanceId(void)                                       override { return m_instanceId; }
	virtual int getModelInstanceId(void)                                  override { return m_modelInstanceId; }
	virtual float getSampleRate(void)                                     override { return m_inputSampleRate; }
	virtual const char* getPartialResult(void)                            override;
	virtual const char* getFinalResult(void)                              override;
	virtual bool getPartialStatus(void)                                   override;
	virtual std::unique_ptr<RecognizedUtterance> getFinalResultData(void) override;
	virtual int getFrameResolution(void)                                  override;

	// TBD move to base?
	void resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood);
	
private:
	static const ssize_t m_processingSampleRate = 16000;
	
	static int voskRecognizerInstanceId;

	bool m_libraryLoaded;
	uint64_t m_vadFrameCounter;
	
	std::thread *recoWorkerThread;
	bool threadRunning;
	std::deque<std::unique_ptr<AudioPacket>> audioPackets;
	std::mutex audioPacketMutex;
	std::condition_variable audioPacketNotify;
	void workerThreadFunc(void);

	std::vector<float> pcmf32;
	bool pcmBufferFragmented;
	std::unique_ptr<VADFrameTiming> currFragmentStartTime;
		
	char* leftOverData;
	int leftOverDataLen = 0;
	
	void runTokensToWords(void);

	// to avoid early deletion of string objects, use preallocated memory for the most recent string
	char partialResultBuffer[1000];
	char finalResultBuffer[100000];
	
	void promoteToFinalResult(std::unique_ptr<VADFrameTiming> currStart, std::unique_ptr<VADFrameTiming> currStop);
	void runWhisper(struct whisper_context* ctx);
	
};

#endif // VOSK_RECOGNIZER_H

