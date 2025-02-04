
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
VoskRecognizer::VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness)
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

    vad = new VADWrapper(aggressiveness, m_processingSampleRate, 5, 5, 5, 5);
	m_vadFrameCounter = 0;
    
    audioLogger = new AudioLogger(std::string(PREFIX "logs/"), m_instanceId);
    
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
    cpp = new CustomPostProc(true, replacement_file, true);
    
    threadRunning = true;
    recoWorkerThread = new std::thread(&VoskRecognizer::workerThreadFunc, this);
    
    lastUttStopTime = 0;
    longPauseBetweenUtterances = true;
}

//////////////////////////////////////////////
VoskRecognizer::~VoskRecognizer(void)
{
	// clear audio queue and finalize thread
	std::unique_lock<std::mutex> audioPacketLock{audioPacketMutex};
	audioPackets.clear();
	threadRunning = false;
	audioPacketLock.unlock();
	audioPacketNotify.notify_one();
	recoWorkerThread->join();
	delete(recoWorkerThread);

	// now we can free all resources
	
	delete(cpp);
	delete(hpp);
	
	std::cout << "vosk_recognizer_free, instance=" << m_instanceId << std::endl;
	
	delete(audioLogger);
	
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
void VoskRecognizer::setTimeStamp(int64_t seconds, int64_t uSeconds)
{
	std::cout << "TIMESTAMP: " << seconds << "." << uSeconds << "s" << std::endl;
}

//////////////////////////////////////////////
int VoskRecognizer::acceptWaveform(const char *data, int length)
{
	int retVal;
	
	if ((m_inputSampleRate != 48000) || (m_processingSampleRate != 16000))
	{
		// only 48kHz-->16kHz is supported (both VAD and recognizer)
		// e.g. Jitsi provides 48 kHz so we need to downsample 1:3
		std::cout << "Unsupported sampling rates input " << m_inputSampleRate << " Hz and processing " << m_processingSampleRate << "Hz." << std::endl;
		assert(false);	
	}

	// create object and copy all data
	std::unique_ptr packet = std::make_unique<AudioPacket>();
	packet->length      = length;
	packet->data        = new char[length];
	packet->arrivalTime = std::chrono::system_clock::now();
	memcpy(packet->data, data, length);
	
	// push to queue and notify worker
	std::unique_lock<std::mutex> audioPacketLock{audioPacketMutex};
	audioPackets.push_back(std::move(packet));
	audioPacketLock.unlock();
	audioPacketNotify.notify_one();
	
	// std::cout << "acceptWaveform push -->" << std::endl;
			
	// access final results queue to compute return value
    finalResultMutex.lock();
    
	if (finalResults.size() > 0)
	{
		// at least one final utterance can be read
		retVal = 1;
	}
	else
	{
		// no final utterance available (maybe partial)
		retVal = 0;
	}
	
	finalResultMutex.unlock();
	
	return retVal;
}

//////////////////////////////////////////////
bool VoskRecognizer::getRecognizerBusy(bool audioQueueOnly)
{
	bool busy = false;
	
	std::unique_lock<std::mutex> audioPacketLock{audioPacketMutex};
	if (audioQueueOnly == true)
	{
		// poll input queue only
		return 	(audioPackets.size() > 2) ? true : false;
	}

	// polling for finished
	
	if (audioPackets.size() > 0)
	{
		busy = true;
	}
	audioPacketLock.unlock();
	
	//
	
	partialResultMutex.lock();
	
	if (partialResult.size() > 0)
	{
		busy = true;
	}
	
	partialResultMutex.unlock();
	
	//
	
    finalResultMutex.lock();
    
	if (finalResults.size() > 0)
	{
		busy = true;
	}
	
    finalResultMutex.unlock();
    
	return busy;
}

//////////////////////////////////////////////
void VoskRecognizer::workerThreadFunc(void)
{
	char initStatus;
	
	bool threadAlive;
	
	int status;
	bool noMoreData;

	const int framelen48=480;
	const int framelen16=160;
	int32_t tmp[framelen48 + 256] = { 0 };
	int16_t buf[framelen16];
	
	initStatus = cfgikts_load(m_configPath.c_str(), &recikts_cfg);
	checkRecognizerError(initStatus, "cfgikts_load");
		
	initStatus = recikts_start(recikts_cfg);
	checkRecognizerError(initStatus, "recikts_start");
		
	WebRtcSpl_ResetResample48khzTo16khz(&m_resamplestate_48_to_16);

	m_recoState = VoskRecognizerState::INIT;
	
	std::cout << "RECO_THREAD cfg load OK" << std::endl;

	threadAlive = true;
	while (threadAlive == true)
	{
		std::unique_lock<std::mutex> audioPacketLock{audioPacketMutex};
		
		if (audioPackets.size() > 0)
		{
			std::unique_ptr<AudioPacket> packet = std::move(audioPackets.front());
			audioPackets.pop_front();

			audioPacketLock.unlock();
		
			// std::cout << "RECO_THREAD <-- pop" << std::endl;

			char *data = packet->data;
			int length = packet->length;
			std::chrono::time_point<std::chrono::system_clock> arrivalTime = packet->arrivalTime;
			
			// splitting audio into chunks & resampling to 16kHz
			while(leftOverDataLen + length >= framelen48 * 2){
		
				int useLen = framelen48 * 2 - leftOverDataLen;
				memcpy(leftOverData + leftOverDataLen, data, useLen);
				data += useLen;
				length -= useLen;
				leftOverDataLen = 0;
		
				WebRtcSpl_Resample48khzTo16khz((const int16_t*)leftOverData,buf,&m_resamplestate_48_to_16,tmp);
		  
				// TODO we could remove all leftover handling from VAD
				status = vad->process(m_processingSampleRate, buf, framelen16, m_vadFrameCounter++, arrivalTime);
			
				if (status == -1)
				{
					std::cout << "VAD processing error!" << std::endl;	
				}
				
				// every VAD frame covers 10ms of audio
				arrivalTime += std::chrono::milliseconds(10);
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
				VADWrapperState uttStatus;
				bool detectedUttFinished = false;
				
				assert(availableChunks > 0);
				
				// std::cout << "Processing " << availableChunks << " announced chunks." << std::endl;
				
				while (availableChunks > 0)
				{
					uttStatus = vad->getUtteranceStatus();
					
					std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk = vad->getNextChunk();
					
					status = recikts_audio(chunk->samples, VADWrapper::nrVADSamples);
							checkRecognizerError(status, "recikts_audio");
					
					audioLogger->addChunk(std::move(chunk));
					
					availableChunks--;
					
					// once the last postbuf chunk is read, state goes back to idle and we have a complete utterance
					if ((uttStatus == VADWrapperState::POSTBUF) && (vad->getUtteranceStatus() == VADWrapperState::IDLE))
					{
						detectedUttFinished = true;	
					}
					
					// std::cout << "Push chunks to recognizer, remaining = " << availableChunks << std::endl;
				}
		
				// here we assume that all callbacks from recikts have happened and there is nothing pending
				if (detectedUttFinished == true)
				{
					// flush results, but don't indicate new speaker yet
					recikts_restart(0);
					
					promoteToFinalResult();

					// restart again but now consider the hint whether speaker has changed
					recikts_restart((longPauseBetweenUtterances == true) ? 1 : 0);
				}
		
				noMoreData = vad->analyze();
			}
		} 
		else
		{
			if (threadRunning == false)
			{
				threadAlive = false;
				audioPacketLock.unlock();
			}
			else
			{
				// lock is still held
				audioPacketNotify.wait(audioPacketLock);
			}
		}		
	}
		
	promoteToFinalResult();
	
    initStatus = recikts_stop();
    checkRecognizerError(initStatus, "recikts_stop");
                
    initStatus = cfgikts_free(&recikts_cfg);
    checkRecognizerError(initStatus, "cfgikts_free");
	
	m_recoState = VoskRecognizerState::UNINIT;
	
	std::cout << "RECO_THREAD goodbye" << std::endl;
}

//////////////////////////////////////////////
const char* VoskRecognizer::getPartialResult(void)
{
	std::string res = "{ \"partial\" : \"";
	
	partialResultMutex.lock();
	
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
	
	partialResultMutex.unlock();
	
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
bool VoskRecognizer::getPartialStatus(void)
{
	return ((vad->getUtteranceStatus() != VADWrapperState::IDLE) ? true : false);
}

//////////////////////////////////////////////
const char* VoskRecognizer::getFinalResult(void)
{
	std::string res = "{ \"text\" : \"-- ";
    int64_t uStartTime = 0;
    int64_t uStartTimeMs = 0;
    int64_t uStopTime = 0;
    int64_t uStopTimeMs = 0;
	
    finalResultMutex.lock();
    
	if (finalResults.size() > 0)
	{
		std::unique_ptr<FinalResult> fin = std::move(finalResults.front());
		finalResults.pop_front();
		res += fin->text;
		
		uStartTime   = fin->uStartTime;
		uStartTimeMs = fin->uStartTimeMs;
		uStopTime    = fin->uStopTime;
		uStopTimeMs  = fin->uStopTimeMs;
	}
	
    finalResultMutex.unlock();
    
	if (detailedResults == false)
	{
		res += " --\" }";
	}
	else
	{
		res += " --\", \"start\" : \"";
		res += std::to_string(uStartTime);
		res += "\", \"startMs\" : \"";
		res += std::to_string(uStartTimeMs);
		res += "\", \"stop\" : \"";
		res += std::to_string(uStopTime);
		res += "\", \"stopMs\" : \"";
		res += std::to_string(uStopTimeMs);
		res += "\" }";
	}
		
	std::cout << "Final result: " << res << std::endl;
	
	memset(finalResultBuffer, 0, sizeof(finalResultBuffer));
	strncpy(finalResultBuffer, res.c_str(), sizeof(finalResultBuffer) - 1);
	
	return finalResultBuffer;	
}

//////////////////////////////////////////////
std::unique_ptr<FinalResult> VoskRecognizer::getFinalResultData(void)
{
	std::unique_ptr<FinalResult> res = std::make_unique<FinalResult>();
	
    finalResultMutex.lock();
    
	if (finalResults.size() > 0)
	{
		res = std::move(finalResults.front());
		finalResults.pop_front();
	}
	
    finalResultMutex.unlock();
    
	return res;
}

//////////////////////////////////////////////
void VoskRecognizer::promoteToFinalResult(void)
{
	std::string finalResult;
	
	partialResultMutex.lock();
	
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
		
		std::unique_ptr<FinalResult> res = std::make_unique<FinalResult>();
		
		if (subword_regex.length() > 0)
		{
			std::regex subword(subword_regex);
			finalResult = std::regex_replace(finalResult, subword, "");
		}
		
		std::cout << "Raw final result: " << finalResult << std::endl;
		
		// try to fix various shortcomings of the result
		std::string spellResult = hpp->processLine(cpp->processLine(finalResult));

		audioLogger->flush(spellResult);
				
		res->text = spellResult;
		
		res->frameCounterStart = vad->getUtteranceStartFrameCtr();
		res->frameCounterEnd   = vad->getUtteranceStopFrameCtr();
		
		res->uStartTime   = vad->getUtteranceStart();
		res->uStartTimeMs = vad->getUtteranceStartMs();
		res->uStopTime    = vad->getUtteranceStop();
		res->uStopTimeMs  = vad->getUtteranceStopMs();
		
		// calculate the hint whether the speaker has changed
		if (res->uStartTime >= lastUttStopTime)
		{
			if ((res->uStartTime - lastUttStopTime) > longPauseSeconds)
			{
				std::cout << ">>>> HINT: new speaker <<<<" << std::endl;
				longPauseBetweenUtterances = true;
			}
			else
			{
				std::cout << "<<<< HINT: speaker unchanged >>>>" << std::endl;
				longPauseBetweenUtterances = false;
			}
		}
		lastUttStopTime = res->uStopTime;
		
		finalResultMutex.lock();
		
		finalResults.push_back(std::move(res));
		
		finalResultMutex.unlock();
		
		partialResult.clear();
	}
	
	partialResultMutex.unlock();
}

//////////////////////////////////////////////
void VoskRecognizer::resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood)
{
	std::unique_ptr<RecognitionResult> newResult = std::make_unique<RecognitionResult>(word, startTimeMs, endTimeMs, negLogLikelihood);
	
	partialResult.push_back(std::move(newResult));	
	
	// we assume that the callbacks are only triggered by recikts_audio() so we don't have to guess
	// when an utterance shall be considered final
	/*
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
	*/
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
