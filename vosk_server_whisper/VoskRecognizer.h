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
#include "PartialResultOracle.h"

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
	virtual std::string getVersionStringsResult(void) override;

private:
	static const ssize_t m_processingSampleRate = 16000;
	
	std::vector<float> pcmf32;
	bool pcmBufferFragmented;
	std::unique_ptr<VADFrameTiming> currFragmentStartTime;
		
	char* leftOverData;
	int leftOverDataLen = 0;
	
	PartialResultOracle* partOracle;

	std::string whisper_version;
	std::string whisper_model;
};

#endif // VOSK_RECOGNIZER_H

