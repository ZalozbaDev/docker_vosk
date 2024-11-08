#ifndef RECOGNIZER_BASE_H
#define RECOGNIZER_BASE_H

#include <cstdint>
#include <string>
#include <memory>
#include <chrono>

enum VoskRecognizerState {UNINIT, INIT};

class FinalResult
{
public:
	
	std::string text;
	uint64_t frameCounterStart;
	uint64_t frameCounterEnd;
    int64_t  uStartTime;
    int64_t  uStartTimeMs;
    int64_t  uStopTime;
    int64_t  uStopTimeMs;
};

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
	virtual int getInstanceId(void)                          = 0;
	virtual int getModelInstanceId(void)                     = 0;
	virtual float getSampleRate(void)                        = 0;
	virtual void setDetailedResult(bool detailsOn)           = 0;
	virtual int acceptWaveform(const char *data, int length) = 0;
	virtual bool getRecognizerBusy(bool audioQueueOnly = false) = 0;
	virtual const char* getPartialResult(void) = 0;
	virtual const char* getFinalResult(void) = 0;
	virtual bool getPartialStatus(void) = 0;
	virtual std::unique_ptr<FinalResult> getFinalResultData(void) = 0;
	virtual ~RecognizerBase();

};

#endif // RECOGNIZER_BASE_H

