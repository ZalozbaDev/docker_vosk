#ifndef RECOGNIZER_BASE_H
#define RECOGNIZER_BASE_H

#include <cstdint>
#include <string>
#include <memory>
#include <chrono>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "RecognitionResult.h"

#include <VADWrapper.h>
#include <Resampler.h>
#include <RecognitionResult.h>
#include <AudioLogger.h>

#include <HunspellPostProc.h>
#include <CustomPostProc.h>


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
	RecognizerBase(int modelId, float sample_rate, const char *configPath, int aggressiveness, const ssize_t processingSampleRate);
	virtual ssize_t getProcessingSampleRate(void)  = 0;
	virtual ~RecognizerBase();
	
	int getInstanceId(void)       { return m_instanceId; }
	int getModelInstanceId(void)  { return m_modelInstanceId; }
	float getSampleRate(void)     { return m_inputSampleRate; }
	
	std::string getLocalTimeStamp(void);
	void setDetailedResult(bool detailsOn);
	void setTimeStamp(int64_t seconds, int64_t uSeconds);
	bool getRecognizerBusy(bool audioQueueOnly = false);
	
	int acceptWaveform(const char *data, int length);
	bool getPartialStatus(void);
	const char* getPartialResult(void);
	const char* getFinalResult(void);
	std::unique_ptr<RecognizedUtterance> getFinalResultData(void);
	
	int getFrameResolution(void);
	
protected:
	static int voskRecognizerInstanceId;
	
	bool detailedResults;
	
	std::chrono::time_point<std::chrono::system_clock> clientTimeStamp;
	
	int m_instanceId;
	int m_modelInstanceId;
	float m_inputSampleRate;

	VoskRecognizerState m_recoState;
	std::string m_configPath;

	AudioLogger *audioLogger;
	
	HunspellPostProc *hpp;
	CustomPostProc *cpp;

	VADWrapper *vad;
	Resampler  *resample;

	uint64_t m_vadFrameCounter;
	
	std::thread *recoWorkerThread;
	bool threadRunning;
	std::deque<std::unique_ptr<AudioPacket>> audioPackets;
	std::mutex audioPacketMutex;
	std::condition_variable audioPacketNotify;
	virtual void workerThreadFunc(void) = 0;
	
	std::vector<std::unique_ptr<RecognizedToken>>    tokens;
	std::mutex tokenMutex;
	
	std::vector<std::unique_ptr<RecognizedWord>>     words;
	std::mutex wordMutex;
	
	std::deque<std::unique_ptr<RecognizedUtterance>> utterances;
	std::mutex utteranceMutex;
	
	virtual void runTokensToWords(void) = 0;
	
	void promoteToFinalResult(std::unique_ptr<VADFrameTiming> currStart, std::unique_ptr<VADFrameTiming> currStop);

	// to avoid early deletion of string objects, use preallocated memory for the most recent string
	char partialResultBuffer[1000];
	char finalResultBuffer[100000];
	
private:

};

#endif // RECOGNIZER_BASE_H

