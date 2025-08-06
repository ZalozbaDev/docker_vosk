#ifndef VOSK_RECOGNIZER_H
#define VOSK_RECOGNIZER_H

#include "RecognizerBase.h"

#include <iostream>

#include <queue>

#include <thread>
#include <mutex>
#include <condition_variable>

extern "C" {
#include "recikts.h"
#include "vosk_api.h"
}

#include <VADWrapper.h>
#include <Resampler.h>
#include <RecognitionResult.h>
#include <AudioLogger.h>

#include <HunspellPostProc.h>
#include <CustomPostProc.h>

//////////////////////////////////////////////
class VoskRecognizer:public RecognizerBase
{
public:
	VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness=2);
	virtual ~VoskRecognizer(void);
	
	virtual int getInstanceId(void)                               override { return m_instanceId; }
	virtual int getModelInstanceId(void)                          override { return m_modelInstanceId; }
	virtual float getSampleRate(void)                             override { return m_inputSampleRate; }
	virtual void setDetailedResult(bool detailsOn)                override;
	virtual int acceptWaveform(const char *data, int length)      override;
	virtual bool getRecognizerBusy(bool audioQueueOnly = false)   override;
	virtual const char* getPartialResult(void)                    override;
	virtual const char* getFinalResult(void)                      override;
	virtual bool getPartialStatus(void)                           override;
	virtual std::unique_ptr<FinalResult> getFinalResultData(void) override;
	virtual int getFrameResolution(void)                          override;

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
	
	std::string m_configPath;

	void *libmInstance;
	void *recInstance;
	
	void loadLibrary(void);
	void unloadLibrary(void);
	void libraryError(void);
	void delayedInitialization(void);
	
	void checkRecognizerError(char status, const char *functionName) {
		if (status != 1) 
		{
			char tmp[1000];
			
			std::cout << functionName << " error val=" << status << "text=";
			
			while(recikts_err(tmp,sizeof(tmp))) 
			{
				std::string str = tmp;
				std::cout << " " << tmp << " ";
			}
			
			std::cout << "." << std::endl; 
		}
	}
	
	const char* (*recikts_version)();
	char        (*recikts_callback_register)(recikts_callback_fnc fnc, void *userdata);
	char        (*cfgikts_load)(const char *fn, struct cfgikts *cfg);
	char        (*recikts_start)(struct cfgikts cfg);
	char        (*recikts_audio)(int16_t* buf,uint32_t samples);
	char        (*recikts_restart)(char);
	char        (*recikts_stop)();
	char        (*cfgikts_free)(struct cfgikts *cfg);
	char        (*recikts_err)(char* buf,int size);
	
	std::thread *recoWorkerThread;
	bool threadRunning;
	std::deque<std::unique_ptr<AudioPacket>> audioPackets;
	std::mutex audioPacketMutex;
	std::condition_variable audioPacketNotify;
	void workerThreadFunc(void);
	
	struct cfgikts recikts_cfg;
	
	static void recikts_callback(struct recikts_callback_dat dat, void *userdata);
	
	VADWrapper *vad;
	Resampler  *resample;

	char* leftOverData;
	int leftOverDataLen = 0;
	
	std::chrono::time_point<std::chrono::system_clock> clientTimeStamp;
	
	std::vector<std::unique_ptr<RecognitionResult>> partialResult;
	std::mutex partialResultMutex;
	
	std::deque<std::unique_ptr<FinalResult>>        finalResults;
	std::mutex finalResultMutex;
	
	// to avoid early deletion of string objects, use preallocated memory for the most recent string
	char partialResultBuffer[1000];
	char finalResultBuffer[1000];
	bool detailedResults;
	
	void promoteToFinalResult(void);
	
	AudioLogger *audioLogger;
	
	std::string subword_regex;
	
	HunspellPostProc *hpp;
	CustomPostProc *cpp;
	
	static const int64_t longPauseSeconds = 10;
	int64_t lastUttStopTime;
	bool longPauseBetweenUtterances;
};

#endif // VOSK_RECOGNIZER_H

