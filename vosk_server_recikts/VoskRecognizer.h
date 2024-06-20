#ifndef VOSK_RECOGNIZER_H
#define VOSK_RECOGNIZER_H

#include <iostream>

extern "C" {
#include "recikts.h"
#include "vosk_api.h"
}

#include <VADWrapper.h>
#include <RecognitionResult.h>
#include <AudioLogger.h>
extern "C" {
#include "common_audio/signal_processing/include/signal_processing_library.h"
}

#include <HunspellPostProc.h>
#include <CustomPostProc.h>

enum VoskRecognizerState {UNINIT, INIT};

//////////////////////////////////////////////
class VoskRecognizer
{
public:
	VoskRecognizer(int modelId, float sample_rate, const char *configPath);
	~VoskRecognizer(void);
	int getInstanceId(void) { return m_instanceId; }
	int getModelInstanceId(void) { return m_modelInstanceId; }
	float getSampleRate(void) { return m_inputSampleRate; }
	void setDetailedResult(bool detailsOn);
	int acceptWaveform(const char *data, int length);
	void resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood);
	const char* getPartialResult(void);
	const char* getFinalResult(void);
	
private:
	static const ssize_t m_processingSampleRate = 16000;
	
	static int voskRecognizerInstanceId;

	int m_instanceId;
	int m_modelInstanceId;
	float m_inputSampleRate;
	bool m_libraryLoaded;
	VoskRecognizerState m_recoState;
	
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
	char        (*recikts_restart)();
	char        (*recikts_stop)();
	char        (*cfgikts_free)(struct cfgikts *cfg);
	char        (*recikts_err)(char* buf,int size);
	
	struct cfgikts recikts_cfg;
	
	static void recikts_callback(struct recikts_callback_dat dat, void *userdata);
	
	VADWrapper *vad;

	WebRtcSpl_State48khzTo16khz m_resamplestate_48_to_16;
	char leftOverData[480*2] = {0};
	int leftOverDataLen = 0;
	
	std::vector<std::unique_ptr<RecognitionResult>> partialResult;
	
	std::vector<std::string>                        finalResults;
	
	// to avoid early deletion of string objects, use preallocated memory for the most recent string
	char partialResultBuffer[1000];
	char finalResultBuffer[1000];
	bool detailedResults;
	
	void promoteToFinalResult(void);
	
	AudioLogger *audioLogger;
	
	std::string subword_regex;
	
	HunspellPostProc *hpp;
	CustomPostProc *cpp;
};

#endif // VOSK_RECOGNIZER_H

