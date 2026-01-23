#ifndef VOSK_RECOGNIZER_H
#define VOSK_RECOGNIZER_H

#include "RecognizerBase.h"

#include <iostream>

#include <queue>

#include <thread>
#include <mutex>
#include <condition_variable>

extern "C" {
#include "vosk_api.h"
}

#include "RecIKTSImpl.h"

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
	
	char* leftOverData;
	int leftOverDataLen = 0;
	
	RecIKTSImpl* recIktsImpl;
	
	void promoteToFinalResult(std::unique_ptr<VADFrameTiming> currStart, std::unique_ptr<VADFrameTiming> currStop);
	
	static const int64_t longPauseSeconds = 10;
	int64_t lastUttStopTime;
	bool longPauseBetweenUtterances;
};

#endif // VOSK_RECOGNIZER_H

