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
	
protected:
	virtual void runTokensToWords(void) override;
	virtual void workerThreadFunc(void) override;

private:
	static const ssize_t m_processingSampleRate = 16000;
	
	std::vector<float> pcmf32;
	bool pcmBufferFragmented;
	std::unique_ptr<VADFrameTiming> currFragmentStartTime;
		
	char* leftOverData;
	int leftOverDataLen = 0;

	void promoteToFinalResult(std::unique_ptr<VADFrameTiming> currStart, std::unique_ptr<VADFrameTiming> currStop);
};

#endif // VOSK_RECOGNIZER_H

