#include "RecIKTSImpl.h"

#ifndef PREFIX
  #define PREFIX "/"
#endif
#ifndef RECIKTSLIB
  #define RECIKTSLIB "recikts64rel.so"
#endif

//////////////////////////////////////////////
RecIKTSImpl::RecIKTSImpl(std::string configPath)
{
	char initStatus;
	
	m_libraryLoaded   = false;
	
	loadLibrary();
	
	initStatus = recikts_callback_register(RecIKTSImpl::recikts_callback, this);
	checkRecognizerError(initStatus, "recikts_callback_register");
	
	initStatus = cfgikts_load(configPath.c_str(), &recikts_cfg);
	checkRecognizerError(initStatus, "cfgikts_load");
		
	initStatus = recikts_start(recikts_cfg);
	checkRecognizerError(initStatus, "recikts_start");
}

//////////////////////////////////////////////
RecIKTSImpl::~RecIKTSImpl()
{
	char initStatus;
	
    initStatus = recikts_stop();
    checkRecognizerError(initStatus, "recikts_stop");
                
    initStatus = cfgikts_free(&recikts_cfg);
    checkRecognizerError(initStatus, "cfgikts_free");
	
	unloadLibrary();
}

//////////////////////////////////////////////
std::string RecIKTSImpl::getAnnouncementString(void)
{
	std::string versionStr = std::string(recikts_version());
	std::string modelStr = std::regex_replace(m_modelPath, std::regex("(\\/|\\.)"), "-");
	
	return versionStr + " : " + modelStr;
}

//////////////////////////////////////////////
void RecIKTSImpl::consume(int16_t* buf,uint32_t samples)
{
	char status;
	if (m_libraryLoaded == true)
	{
		status = recikts_audio(buf, samples)
		checkRecognizerError(status, "recikts_audio");
	}
	else
	{
		std::cout << "recikts library not loaded, no recognition!" << std::endl;	
	}
}

//////////////////////////////////////////////
void RecIKTSImpl::flush(bool longPauseBetweenUtterances)
{
	char status;
	if (m_libraryLoaded == true)
	{
		recikts_restart((longPauseBetweenUtterances == true) ? 1 : 0);		
		checkRecognizerError(status, "recikts_restart");
	}
	else
	{
		std::cout << "recikts library not loaded, no recognition!" << std::endl;	
	}
}

//////////////////////////////////////////////
void VoskRecognizer::loadLibrary(void)
{
	int status;
	Lmid_t newlmid;
	
	libmInstance = dlmopen(LM_ID_NEWLM, "/lib/x86_64-linux-gnu/libm.so.6", RTLD_NOW);
	if (libmInstance != NULL)
	{
		status = dlinfo(libmInstance, RTLD_DI_LMID, &newlmid);
		
		if (status == 0)
		{
			recInstance = dlmopen(newlmid, PREFIX RECIKTSLIB, RTLD_NOW);
			
			if (recInstance != NULL)
			{
				recikts_version           = (const char* (*)())                     dlsym(recInstance, "recikts_version");
				recikts_callback_register = (char (*)(recikts_callback_fnc, void*)) dlsym(recInstance, "recikts_callback_register");
				cfgikts_load              = (char (*)(const char*, cfgikts*))       dlsym(recInstance, "cfgikts_load");
				recikts_start             = (char (*)(cfgikts))                     dlsym(recInstance, "recikts_start");
				recikts_audio             = (char (*)(int16_t*, uint32_t))          dlsym(recInstance, "recikts_audio");
				recikts_restart           = (char (*)(char))                        dlsym(recInstance, "recikts_restart");
				recikts_stop              = (char (*)())                            dlsym(recInstance, "recikts_stop");
				cfgikts_free              = (char (*)(cfgikts*))                    dlsym(recInstance, "cfgikts_free");
				recikts_err               = (char (*)(char*, int))                  dlsym(recInstance, "recikts_err");
				
				if ((recikts_version != NULL)   && (recikts_callback_register != NULL) &&
					(cfgikts_load != NULL)  && (recikts_start != NULL) &&
					(recikts_audio != NULL) && (recikts_restart != NULL) &&
					(recikts_stop != NULL)  && (cfgikts_free != NULL) &&
					(recikts_err != NULL))
				{
					m_libraryLoaded = true;
				}
				
				if (m_libraryLoaded == false)
				{
					std::cout << "One or more functions from the recikts library could not be resolved!" << std::endl;
					libraryError();
					dlclose(recInstance);	
				}
			}
		}
		
		if (m_libraryLoaded == false)
		{
			libraryError();
			dlclose(libmInstance);	
		}
	}
	
	if (m_libraryLoaded == false)
	{
		libraryError();	
	}
}

//////////////////////////////////////////////
void VoskRecognizer::libraryError(void)
{
	char* err = dlerror();
	
	if (err == NULL)
	{
		std::cout << "No error occurred for last library operation." << std::endl;
	} 
	else 
	{
		std::cout << err << std::endl;		
	}
}

//////////////////////////////////////////////
void VoskRecognizer::unloadLibrary(void)
{
	int status;
	
	status = dlclose(recInstance);
	if (status != 0) libraryError();
	
	status = dlclose(libmInstance);
	if (status != 0) libraryError();
	
	m_libraryLoaded = false;
}


//////////////////////////////////////////////
void RecIKTSImpl::checkRecognizerError(char status, const char *functionName) 
{
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

//////////////////////////////////////////////
void VoskRecognizer::resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood)
{
	std::unique_ptr<RecognitionResult> newResult = std::make_unique<RecognitionResult>(word, startTimeMs, endTimeMs, negLogLikelihood);
	
	partialResult.push_back(std::move(newResult));	
}

//////////////////////////////////////////////
//
// this is the C-style callback from the library, which is 
// immediately routed to the callback from the instance of this class
//
//////////////////////////////////////////////
void RecIKTSImpl::recikts_callback(struct recikts_callback_dat dat, void *userdata){
	RecIKTSImpl* inst;
	
	if(dat.word[0]){
		printf("Result [%i-%i ms]: %s [%.1f]\n",dat.tstart,dat.tend,dat.word,dat.nld);
		fflush(stdout);

		inst = static_cast<RecIKTSImpl*>(userdata);
		inst->resultCallback(dat.word, dat.tstart, dat.tend, dat.nld);
	}
}
