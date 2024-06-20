
#include <VoskRecognizer.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <cassert>

#include <regex>

#ifndef PREFIX
  #define PREFIX "/"
#endif
#ifndef RECIKTSLIB
  #define RECIKTSLIB "recikts64rel.so"
#endif

int VoskRecognizer::voskRecognizerInstanceId = 1;

//////////////////////////////////////////////
VoskRecognizer::VoskRecognizer(int modelId, float sample_rate, const char *configPath)
{
	char status;
	
	std::cout << "vosk_recognizer_new, instance=" << voskRecognizerInstanceId << " sample_rate=" << sample_rate << std::endl;

	m_modelInstanceId = modelId;
	m_instanceId      = voskRecognizerInstanceId++;
	m_inputSampleRate = sample_rate;
	
	m_libraryLoaded   = false;
	
	m_recoState = VoskRecognizerState::UNINIT;
	
	loadLibrary();
	
	status = recikts_callback_register(VoskRecognizer::recikts_callback, this);
	checkRecognizerError(status, "recikts_callback_register");
	
	std::cout << recikts_version() << std::endl;
	
	m_configPath = std::string(configPath);
	
//	status = cfgikts_load(configPath, &recikts_cfg);
//	checkRecognizerError(status, "cfgikts_load");
	
//	status = recikts_start(recikts_cfg);
//	checkRecognizerError(status, "recikts_start");

    // init static parts already here

    vad = new VADWrapper(3, m_processingSampleRate);
	
    audioLogger = new AudioLogger(std::string(PREFIX "/logs/"), m_instanceId);
    
    if (const char *env_p = std::getenv("VOSK_LOG_AUDIO"))
    {
        if (strcasecmp(env_p, "True") == 0)
        {
        	audioLogger->activate();	
        }
    }
    
    if (const char *env_p = std::getenv("VOSK_SUBWORD_REGEX"))
    {
    	subword_regex = std::string(env_p);
    	std::cout << "Using regex '" << subword_regex << "' for subword  merging." << std::endl;
    }
    else
    {
    	subword_regex = std::string("");	
    }

    hpp = new HunspellPostProc("", "", "");

    std::string replacement_file = "";
    if (const char *env_p = std::getenv("VOSK_REPLACEMENT_FILE"))
    {
    	replacement_file = env_p;
    }
    cpp = new CustomPostProc(true, replacement_file);
}

//////////////////////////////////////////////
VoskRecognizer::~VoskRecognizer(void)
{
	char status;
	
	delete(cpp);
	delete(hpp);
	
	std::cout << "vosk_recognizer_free, instance=" << m_instanceId << std::endl;
	
	delete(audioLogger);
	
	// if not yet initalized, do not try to shut down either
	if (m_recoState == VoskRecognizerState::UNINIT)
	{
                status = recikts_stop();
                checkRecognizerError(status, "recikts_stop");
                
                status = cfgikts_free(&recikts_cfg);
                checkRecognizerError(status, "cfgikts_free");
	}
	
	m_recoState = VoskRecognizerState::UNINIT;
	
	unloadLibrary();
	
	delete(vad);
	
	partialResult.clear();
	finalResults.clear();
	
	// don't decrease, let every instance get a unique ID
	// voskRecognizerInstanceId--;
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
				recikts_restart           = (char (*)())                            dlsym(recInstance, "recikts_restart");
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
void VoskRecognizer::setDetailedResult(bool detailsOn)
{
	if (detailsOn == true)
	{
		detailedResults = true;	
	}
	else
	{
		detailedResults = false;	
	}
}

//////////////////////////////////////////////
int VoskRecognizer::acceptWaveform(const char *data, int length)
{
	int status;
	bool noMoreData;

	// if not yet initalized, do that here and discard this audio
	if (m_recoState == VoskRecognizerState::UNINIT)
	{
		char initStatus;
		
		initStatus = cfgikts_load(m_configPath.c_str(), &recikts_cfg);
		checkRecognizerError(initStatus, "cfgikts_load");
		
		initStatus = recikts_start(recikts_cfg);
		checkRecognizerError(initStatus, "recikts_start");
		
		WebRtcSpl_ResetResample48khzTo16khz(&m_resamplestate_48_to_16);

		m_recoState = VoskRecognizerState::INIT;
		
		// TODO check if we can still process the audio by simply not returning here
		return 0;
	}
	
	if ((m_inputSampleRate != 48000) || (m_processingSampleRate != 16000))
	{
		// only 48kHz-->16kHz is supported (both VAD and recognizer)
		// e.g. Jitsi provides 48 kHz so we need to downsample 1:3
		std::cout << "Unsupported sampling rates input " << m_inputSampleRate << " Hz and processing " << m_processingSampleRate << "Hz." << std::endl;
		assert(false);	
	}

	// splitting audio into chunks & resampling to 16kHz
	const int framelen48=480;
	const int framelen16=160;
	int32_t tmp[framelen48 + 256] = { 0 };
	int16_t buf[framelen16];
	
	while(leftOverDataLen + length >= framelen48 * 2){

		int useLen = framelen48 * 2 - leftOverDataLen;
		memcpy(leftOverData + leftOverDataLen, data, useLen);
		data += useLen;
		length -= useLen;
		leftOverDataLen = 0;

		WebRtcSpl_Resample48khzTo16khz((const int16_t*)leftOverData,buf,&m_resamplestate_48_to_16,tmp);
  
		// TODO we could remove all leftover handling from VAD
		status = vad->process(m_processingSampleRate, buf, framelen16);
	
		if (status == -1)
		{
			std::cout << "VAD processing error!" << std::endl;	
		}
	}

	if (length > 0)
	{
		leftOverDataLen=length;
		memcpy(leftOverData,data,length);
	}
	
	noMoreData = vad->analyze();
	
	while (noMoreData == false)
	{
		unsigned int availableChunks = vad->getAvailableChunks();
		VADWrapperState uttStatus = vad->getUtteranceStatus();
		
		while (availableChunks > 0)
		{
			std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk = vad->getNextChunk();
			
			status = recikts_audio(chunk->samples, VADWrapper::nrVADSamples);
	                checkRecognizerError(status, "recikts_audio");
			
			audioLogger->addChunk(std::move(chunk));
			
			availableChunks--;
			
			// std::cout << "Push chunks to recognizer, remaining = " << availableChunks << std::endl;
		}
		
		if (uttStatus == VADWrapperState::COMPLETE)
		{
			recikts_restart();
		}

		noMoreData = vad->analyze();
	}
	
	// if we are in idle state (again), a possible partial result is now final
	if (vad->getUtteranceStatus() == VADWrapperState::IDLE)
	{
		promoteToFinalResult();
	}
	
	if (finalResults.size() > 0)
	{
		// at least one final utterance can be read
		return 1;
	}
	
	// no final utterance available (maybe partial)
	return 0;
}

//////////////////////////////////////////////
const char* VoskRecognizer::getPartialResult(void)
{
	std::string res = "{ \"partial\" : \"";
	
	if (partialResult.size() > 0)
	{
		for (unsigned int i = 0; i < partialResult.size(); i++)
		{
			res += partialResult[i]->text;
			if (i < (partialResult.size() - 1))
			{
				res += " ";
			}
		}
	}
	
	if (detailedResults == false)
	{
		res += "\" }";
	}
	else
	{
		// return whether VAD has triggered (e.g. is collecting samples)
		res += "\", \"listen\" : \"";
		res += ((vad->getUtteranceStatus() != VADWrapperState::IDLE) ? "true" : "false");
		res += "\" }";
	}
		
	if (subword_regex.length() > 0)
	{
		std::regex subword(subword_regex);
		res = std::regex_replace(res, subword, "");
	}
	
	std::cout << "Partial result: " << res << std::endl;
	
	memset(partialResultBuffer, 0, sizeof(partialResultBuffer));
	strncpy(partialResultBuffer, res.c_str(), sizeof(partialResultBuffer) - 1);
	
	return partialResultBuffer;	
}

//////////////////////////////////////////////
const char* VoskRecognizer::getFinalResult(void)
{
	std::string res = "{ \"text\" : \"-- ";
	
	if (finalResults.size() > 0)
	{
		std::string currFinalResult = finalResults.front();
		
		if (subword_regex.length() > 0)
		{
			std::regex subword(subword_regex);
			currFinalResult = std::regex_replace(currFinalResult, subword, "");
		}
		
		std::cout << "Raw final result: " << currFinalResult << std::endl;
		
		// try to fix various shortcomings of the result
		std::string spellResult = hpp->processLine(cpp->processLine(currFinalResult));

		audioLogger->flush(spellResult);
		res += spellResult;
		finalResults.erase(finalResults.begin());
	}
	
	if (detailedResults == false)
	{
		res += " --\" }";
	}
	else
	{
		res += " --\", \"start\" : \"";
		res += std::to_string(vad->getUtteranceStart());
		res += "\", \"stop\" : \"";
		res += std::to_string(vad->getUtteranceStop());
		res += "\" }";
	}
		
	std::cout << "Final result: " << res << std::endl;
	
	memset(finalResultBuffer, 0, sizeof(finalResultBuffer));
	strncpy(finalResultBuffer, res.c_str(), sizeof(finalResultBuffer) - 1);
	
	return finalResultBuffer;	
}

//////////////////////////////////////////////
void VoskRecognizer::promoteToFinalResult(void)
{
	std::string finalResult;
	
	if (partialResult.size() > 0)
	{
		for (unsigned int i = 0; i < partialResult.size(); i++)
		{
			finalResult += partialResult[i]->text;
			if (i < (partialResult.size() - 1))
			{
				finalResult += " ";
			}
		}
		
		std::cout << "Promoting partial result to final: " << finalResult << std::endl;
		
		finalResults.push_back(finalResult);
		
		partialResult.clear();
	}
}

//////////////////////////////////////////////
void VoskRecognizer::resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood)
{
	std::unique_ptr<RecognitionResult> newResult = std::make_unique<RecognitionResult>(word, startTimeMs, endTimeMs, negLogLikelihood);
	
	if (partialResult.size() == 0)
	{
		partialResult.push_back(std::move(newResult));	
	}
	else
	{
		// timestamps hopefully show if a new utterance starts (e.g. last one has been flushed)
		if (newResult->start < partialResult.back()->end)
		{
			promoteToFinalResult();
		}

		partialResult.push_back(std::move(newResult));		
	}
}

//////////////////////////////////////////////
void VoskRecognizer::recikts_callback(struct recikts_callback_dat dat, void *userdata){
	VoskRecognizer* inst;
	
	if(dat.word[0]){
		printf("Result [%i-%i ms]: %s [%.1f]\n",dat.tstart,dat.tend,dat.word,dat.nld);
		fflush(stdout);

		inst = static_cast<VoskRecognizer*>(userdata);
		inst->resultCallback(dat.word, dat.tstart, dat.tend, dat.nld);
	}
}
