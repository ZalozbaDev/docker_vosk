#ifndef RECOGNIZER_BASE_H
#define RECOGNIZER_BASE_H

#include <cstdint>
#include <string>
#include <memory>
#include <chrono>

#include "RecognitionResult.h"

enum VoskRecognizerState {UNINIT, INIT};

class AudioPacket
{
public:
	std::chrono::time_point<std::chrono::system_clock> arrivalTime;
	char *data;
	int length;
	
	AudioPacket() {
		data = nullptr;
		length = 0;
	}
	
	~AudioPacket() {
		if (length > 0) {
			delete[] data;
		}
		length = 0;
	}
};

class RecognizerBase
{
public:
	RecognizerBase(int modelId, float sample_rate, const char *configPath, int aggressiveness);
	
	virtual int getInstanceId(void)                                       = 0;
	virtual int getModelInstanceId(void)                                  = 0;
	virtual float getSampleRate(void)                                     = 0;
	virtual int acceptWaveform(const char *data, int length)              = 0;
	virtual bool getRecognizerBusy(bool audioQueueOnly = false)           = 0;
//	virtual void runTokenToWords(void)                                    = 0;
	virtual const char* getPartialResult(void)                            = 0;
	virtual const char* getFinalResult(void)                              = 0;
	virtual bool getPartialStatus(void)                                   = 0;
	virtual std::unique_ptr<RecognizedUtterance> getFinalResultData(void) = 0;
	virtual int getFrameResolution(void)                                  = 0;
	
	virtual ~RecognizerBase();
	std::string getLocalTimeStamp(void);
	void setDetailedResult(bool detailsOn);
	void setTimeStamp(int64_t seconds, int64_t uSeconds);
	
protected:
	static int voskRecognizerInstanceId;
	
	bool detailedResults;
	std::chrono::time_point<std::chrono::system_clock> clientTimeStamp;
	
	int m_instanceId;
	int m_modelInstanceId;
	float m_inputSampleRate;

};

#endif // RECOGNIZER_BASE_H

