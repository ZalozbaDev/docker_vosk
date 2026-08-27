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
	virtual ssize_t getProcessingSampleRate(void)  override { return m_processingSampleRate; }
	virtual void changeConfigPath(const char *newPath) override;
	virtual ~VoskRecognizer(void);

protected:
	virtual void runTokensToWords(void) override;
	void workerThreadFunc(void);
	
private:
	static const ssize_t m_processingSampleRate = 16000;
	
	char* leftOverData;
	int leftOverDataLen = 0;
	
	RecIKTSImpl* recIktsImpl;
	
	static const int64_t longPauseSeconds = 10;
	int64_t lastUttStopTime;
	bool checkUtterancePause;
};

#endif // VOSK_RECOGNIZER_H

