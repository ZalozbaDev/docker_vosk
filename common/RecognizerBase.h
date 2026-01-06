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
	RecognizerBase()                                 = default;
	
	virtual int getInstanceId(void)                                       = 0;
	virtual int getModelInstanceId(void)                                  = 0;
	virtual float getSampleRate(void)                                     = 0;
	virtual void setDetailedResult(bool detailsOn)                        = 0;
	virtual int acceptWaveform(const char *data, int length)              = 0;
	virtual bool getRecognizerBusy(bool audioQueueOnly = false)           = 0;
	virtual void runTokenToWords(void)                                    = 0;
	virtual const char* getPartialResult(void)                            = 0;
	virtual const char* getFinalResult(void)                              = 0;
	virtual bool getPartialStatus(void)                                   = 0;
	virtual std::unique_ptr<RecognizedUtterance> getFinalResultData(void) = 0;
	virtual int getFrameResolution(void)                                  = 0;
	
	virtual ~RecognizerBase();
	std::string getLocalTimeStamp(void);
};

#endif // RECOGNIZER_BASE_H

