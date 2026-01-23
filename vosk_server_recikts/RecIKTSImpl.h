#ifndef RECIKTS_IMPL_H
#define RECIKTS_IMPL_H

#include "RecognitionResult.h"

extern "C" {
#include "recikts.h"
}

class RecIKTSImpl
{
public:
	RecIKTSImpl(std::string configPath);
	std::string getAnnouncementString(void);
	void startUtterance(bool longPauseBetweenUtterances);
	void consumeAudio(int16_t* buf,uint32_t samples);
	void finalizeUtterance();
	void getRecognizedTokens(std::vector<RecognizedToken>& tokens);
	~RecIKTSImpl();
private:
	bool m_libraryLoaded;

	std::string m_configPath;
	
	void loadLibrary(void);
	void unloadLibrary(void);
	void libraryError(void);
	void delayedInitialization(void);
	void checkRecognizerError(char status, const char *functionName);
	
	const char* (*recikts_version)();
	char        (*recikts_callback_register)(recikts_callback_fnc fnc, void *userdata);
	char        (*cfgikts_load)(const char *fn, struct cfgikts *cfg);
	char        (*recikts_start)(struct cfgikts cfg);
	char        (*recikts_audio)(int16_t* buf,uint32_t samples);
	char        (*recikts_restart)(char);
	char        (*recikts_stop)();
	char        (*cfgikts_free)(struct cfgikts *cfg);
	char        (*recikts_err)(char* buf,int size);

	// C and C++ callbacks
	static void recikts_callback(struct recikts_callback_dat dat, void *userdata);
	void resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood);
	
	struct cfgikts recikts_cfg;
	
	void *libmInstance;
	void *recInstance;
	
	std::vector<RecognizedToken> tokens;

};


#endif // RECIKTS_IMPL_H
