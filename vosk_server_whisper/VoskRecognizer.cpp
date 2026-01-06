
#include <VoskRecognizer.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <VADWrapperWebRTC.h>
#include <VADWrapperSilero.h>
#include <ResamplerWebRTC_48_16.h>
#include <ResamplerLibResample_48_16.h>

#include <cassert>
#include <regex>
#include <chrono>

#ifndef WHISPER_MOCK
#include "common.h"
#endif

using namespace std::chrono_literals;

int VoskRecognizer::voskRecognizerInstanceId = 1;

//////////////////////////////////////////////
VoskRecognizer::VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness)
{
	std::cout << "vosk_recognizer_new, instance=" << voskRecognizerInstanceId << " sample_rate=" << sample_rate << std::endl;

	m_modelInstanceId = modelId;
	m_instanceId      = voskRecognizerInstanceId++;
	m_inputSampleRate = sample_rate;
	
	m_recoState = VoskRecognizerState::UNINIT;
	
	m_configPath = std::string(configPath);
	
	detailedResults = false;
	
	std::string helloworld = std::regex_replace(m_configPath, std::regex("(\\/|\\.)"), "-");
	std::unique_ptr<RecognizedUtterance> res = std::make_unique<RecognizedUtterance>();
	res->text = helloworld;
	utterances.push_back(std::move(res));

	// init static parts already here
	
	// adjust pre/post buffers here if needed
	
	audioLogger = new AudioLogger(std::string("logs/"), m_instanceId);
    
    if (const char *env_p = std::getenv("VOSK_LOG_AUDIO"))
    {
        if (strcasecmp(env_p, "True") == 0)
        {
        	audioLogger->activate();	
        }
    }
    
    hpp = new HunspellPostProc("", "", "");

    std::string replacement_file = "";
    if (const char *env_p = std::getenv("VOSK_REPLACEMENT_FILE"))
    {
    	replacement_file = env_p;
    }
    cpp = new CustomPostProc(true, replacement_file, false, 30); // limit to max. 30 characters per second of audio, reduces impact of hallucinations
    
    // optional environment var
    // - --language            ("en", "czech", ...)
    if (const char *env_p = std::getenv("VOSK_MODEL_LANGUAGE"))
    {
    	env_vosk_model_language = env_p;
    }    
    else
    {
    	env_vosk_model_language = "auto";
    }
    std::cout << "ENV setting language to '" << env_vosk_model_language << "'." << std::endl;
    
    // optional environment var
    // - -mc / --max-context   (a.k.a. "n_max_text_ctx":   default = 16384, some models need this to be 0)
    if (const char *env_p = std::getenv("VOSK_WHISPER_MAX_CONTEXT"))
    {
    	env_whisper_max_context = std::atoi(env_p);
    }
    else
    {
    	// use -1 for "don't change default"
    	env_whisper_max_context = -1;
    }
    std::cout << "ENV setting whisper max context to " << env_whisper_max_context << "." << std::endl;
    
    // optional environment var
    // - -nt / --no-timestamps (a.k.a. "print_timestamps": avoid filling t0/t1 [do not call whisper_full_get_segment_tX], 
    //                                                     some models seem to be picky about this - to be investigated)
    env_whisper_no_timestamps = false;
    if (const char *env_p = std::getenv("VOSK_WHISPER_DISABLE_TIMESTAMPS"))
    {
    	if (strcasecmp(env_p, "True") == 0)
    	{
    		env_whisper_no_timestamps = true;
    	}
    }    
    std::cout << "ENV setting whisper no timestamps option to '" << env_whisper_no_timestamps << "'." << std::endl;
    
    // optional environment var
    // - --no_fallback   (do not try to decode in several attempts, as this can slow down decoding on strange audio)
    if (const char *env_p = std::getenv("VOSK_WHISPER_NO_FALLBACK"))
    {
    	if (strcasecmp(env_p, "True") == 0)
    	{
    		env_whisper_no_fallback = true;
    	}
    }
    else
    {
    	// default
    	env_whisper_no_fallback = false;
    }
    std::cout << "ENV setting whisper no fallback to " << env_whisper_no_fallback << "." << std::endl;
    
    if (const char *env_p = std::getenv("VOSK_WHISPER_USE_CPU"))
    {
        if (strcasecmp(env_p, "True") == 0)
        {
        	default_params.use_gpu = false;
        }
    }
    std::cout << "ENV setting whisper use GPU to  " << default_params.use_gpu << "." << std::endl;

    // makes sense to tie the resampler to the VAD algo used - not all combinations are possible anyway
    if (const char *env_p = std::getenv("VOSK_VAD_ALGO"))
    {
        if (strcasecmp(env_p, "Silero") == 0)
        {
        	std::cout << "ENV setting VAD algo to Silero." << std::endl;
        	resample = new ResamplerLibResample_48_16();
        	vad = new VADWrapperSilero(16000, "model/silero_vad.onnx");
        }
        else
        {
        	std::cout << "ENV setting VAD algo to WebRTC." << std::endl;
        	resample = new ResamplerWebRTC_48_16();
        	vad = new VADWrapperWebRTC(aggressiveness, m_processingSampleRate, 15, 15, 5, 5);
        }
    }
    else
    {
       	std::cout << "ENV setting VAD algo to WebRTC." << std::endl;
       	resample = new ResamplerWebRTC_48_16();
    	vad = new VADWrapperWebRTC(aggressiveness, m_processingSampleRate, 15, 15, 5, 5);	
    }
	m_vadFrameCounter = 0;
	    
    threadRunning = true;
    recoWorkerThread = new std::thread(&VoskRecognizer::workerThreadFunc, this);
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
	
	delete(vad);
	delete(resample);
	
	tokens.clear();
	words.clear();
	utterances.clear();
	
	// don't decrease, let every instance get a unique ID
	// voskRecognizerInstanceId--;
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
	// std::cout << "TIMESTAMP: " << seconds << "." << uSeconds << "s" << std::endl;
	clientTimeStamp = std::chrono::system_clock::from_time_t(seconds) + std::chrono::microseconds(uSeconds);
	auto timeStampPrint = std::chrono::system_clock::to_time_t(clientTimeStamp);
	std::cout << "TIMESTAMP: " << std::ctime(&timeStampPrint) << std::endl;
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
	
	tokenMutex.lock();
	if (tokens.size() > 0)
	{
		busy = true;
	}
	tokenMutex.unlock();
	
	//
	
	wordMutex.lock();
	if (words.size() > 0)
	{
		busy = true;
	}
	wordMutex.unlock();
	
	//
	
    utteranceMutex.lock();
	if (utterances.size() > 0)
	{
		busy = true;
	}
    utteranceMutex.unlock();
    
	return busy;
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
    utteranceMutex.lock();
	if (utterances.size() > 0)
	{
		// at least one final utterance can be read
		retVal = 1;
	}
	else
	{
		// no final utterance available (maybe partial)
		retVal = 0;
	}
	utteranceMutex.unlock();
	
	return retVal;
}
	
//////////////////////////////////////////////
void VoskRecognizer::workerThreadFunc(void)
{
	bool threadAlive;
	
	int status;
	bool noMoreData;
	
	struct whisper_context_params cparams;
	struct whisper_context* ctx;

	// splitting audio into chunks & resampling to 16kHz
	// make this depend on the required framelength for VAD
	const int framelen16 = vad->getRequiredFrameLength();
	const int framelen48 = framelen16 * 3;
	int16_t buf[framelen16];
	
	// leftover data buffer should not be bigger than one audio frame
	leftOverData = new char[framelen48 * 2];
	
	// whisper init
	cparams = whisper_context_default_params();
	
	cparams.use_gpu = default_params.use_gpu;
	cparams.flash_attn = false;
	cparams.dtw_token_timestamps = false;
	
	ctx = whisper_init_from_file_with_params(m_configPath.c_str(), cparams);

	pcmf32.clear();
	pcmBufferFragmented = false;
	currFragmentStartTime = std::make_unique<VADFrameTiming>();
		
	m_recoState = VoskRecognizerState::INIT;
	
	///////////////////////
	
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
			
			while(leftOverDataLen + length >= framelen48 * 2){

				int useLen = framelen48 * 2 - leftOverDataLen;
				memcpy(leftOverData + leftOverDataLen, data, useLen);
				data += useLen;
				length -= useLen;
				leftOverDataLen = 0;
				
				resample->resample((const int16_t*)leftOverData, buf, framelen48);
		
				status = vad->process(m_processingSampleRate, buf, framelen16, m_vadFrameCounter++, arrivalTime);
			
				if (status == -1)
				{
					std::cout << "VAD processing error!" << std::endl;	
				}
				
				// every VAD frame covers a defined amount of audio
				arrivalTime += std::chrono::milliseconds(vad->getFrameTimeMs());
			}

			if (length > 0)
			{
				leftOverDataLen=length;
				memcpy(leftOverData,data,length);
			}
			
			noMoreData = vad->analyze((pcmf32.size() < pcm_buffer_short) ? true : false);
			
			while (noMoreData == false)
			{
				unsigned int availableChunks = vad->getAvailableChunks();
				VADWrapperState uttStatus;
				bool detectedUttFinished = false;
				std::unique_ptr<VADFrameTiming> currStart = std::make_unique<VADFrameTiming>();
				std::unique_ptr<VADFrameTiming> currStop = std::make_unique<VADFrameTiming>();
								
				assert(availableChunks > 0);
				
				while (availableChunks > 0)
				{
					uttStatus = vad->getUtteranceStatus();
					
					// get the utterance start and stop properties from VAD wrapper
					if ((currStart->valid == false) && (uttStatus != VADWrapperState::IDLE))
					{
						currStart = vad->getUtteranceStart();
					}
					if ((currStop->valid == false) && (uttStatus == VADWrapperState::POSTBUF))
					{
						currStop = vad->getUtteranceStop();
					}
					
					std::unique_ptr<VADFrame> chunk = vad->getNextChunk();
					
					pcmf32.insert(pcmf32.cend(), chunk->fSamples, chunk->fSamples + chunk->m_numberSamples);
					
					audioLogger->addChunk(std::move(chunk));
					
					availableChunks--;
					
					// once the last postbuf chunk is read, state goes back to idle and we have a complete utterance
					if ((uttStatus == VADWrapperState::POSTBUF) && (vad->getUtteranceStatus() == VADWrapperState::IDLE))
					{
						detectedUttFinished = true;	
					}
				}
				
				if ((detectedUttFinished == true) || (pcmf32.size() > pcm_buffer_max))
				{
					
					runWhisper(ctx);
					
					// first audio buffer
					if (pcmBufferFragmented == false)
					{
						if (detectedUttFinished == true)
						{
							std::cout << ">>>>>>>>>>>>>>>> Fragment false, finished true <<<<<<<<<<<<<<" << std::endl;
							
							// normal utterance end 
							assert(currStart->valid == true);
							assert(currStop->valid == true);
							
							promoteToFinalResult(std::move(currStart), std::move(currStop));
						}
						else
						{
							std::cout << ">>>>>>>>>>>>>>>> Fragment false, finished false <<<<<<<<<<<<<<" << std::endl;
							
							// buffer full --> will fragment!
							assert(currStart->valid == true);
							currStop = vad->getUtteranceCurr();
							
							promoteToFinalResult(std::move(currStart), std::move(currStop));
							
							pcmBufferFragmented = true;
							currFragmentStartTime = vad->getUtteranceCurr();
						}
					}
					// continued audio buffer
					else
					{
						if (detectedUttFinished == true)
						{
							std::cout << ">>>>>>>>>>>>>>>> Fragment true, finished true <<<<<<<<<<<<<<" << std::endl;
														
							// normal utterance end --> end fragmenting
							assert(currFragmentStartTime->valid == true);
							assert(currStop->valid == true);
							
							promoteToFinalResult(std::move(currFragmentStartTime), std::move(currStop));
							
							pcmBufferFragmented = false;
							currFragmentStartTime = std::make_unique<VADFrameTiming>();
						}	
						else
						{
							std::cout << ">>>>>>>>>>>>>>>> Fragment true, finished false <<<<<<<<<<<<<<" << std::endl;
							
							// continue fragmenting
							assert(currFragmentStartTime->valid == true);
							currStop = vad->getUtteranceCurr();
							
							promoteToFinalResult(std::move(currFragmentStartTime), std::move(currStop));
							
							currFragmentStartTime = vad->getUtteranceCurr();
						}
					}
					
					pcmf32.clear();
					
				}
		
				noMoreData = vad->analyze((pcmf32.size() < pcm_buffer_short) ? true : false);
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
	
	pcmf32.clear();

	delete[] leftOverData;
	
	////////////////////////

	whisper_free(ctx);
	
	m_recoState = VoskRecognizerState::UNINIT;
	
	std::cout << "RECO_THREAD goodbye" << std::endl;
}

//////////////////////////////////////////////
void VoskRecognizer::runTokensToWords(void)
{
	tokenMutex.lock();
	
	wordMutex.lock();
	
	std::string currWord = "";
	std::chrono::milliseconds duration = 0ms;
	std::chrono::milliseconds relStart = 0ms;
	std::chrono::milliseconds relEnd   = 0ms;
	std::vector<float>        tokenConfidences;
	bool newWord = true;
	
	for (auto&& token : tokens)
	{
		// check new word
		if ((token->m_text[0] == ' ') && (currWord.length() > 0))
		{
			float confidenceSum = 0.0f;
			for (float val : tokenConfidences) {
				confidenceSum += val;
			}
			float confidenceMean = confidenceSum / tokenConfidences.size();
			
			std::string origWord = cpp->sanitizeWord(currWord);
			std::string replacedWord = cpp->replaceWord(origWord);
			bool spellResult = hpp->spelledCorrectly(replacedWord);
			
			std::unique_ptr<RecognizedWord> word = std::make_unique<RecognizedWord>(
				origWord, replacedWord,
				duration, relStart, relEnd, 
				confidenceMean, spellResult);
			words.push_back(word);
			
			currWord = "";
			duration = 0ms;
			relStart = 0ms;
			relEnd   = 0ms;
			tokenConfidences.clear();
			newWord = true;
		}
		
		// initial space removed when word is stored
		currWord += token->m_text;
		if (newWord == true)
		{
			relStart = token->m_relStart;
			newWord = false;
		}
		duration += token->m_duration;
		relEnd = token->m_relEnd;
		tokenConfidences.push_back(token->m_confidence);
	}
	
	// remaining (sub-)word after all tokens parsed
	if (currWord.length() > 0)
	{
		float confidenceSum = 0.0f;
		for (float val : tokenConfidences) {
			confidenceSum += val;
		}
		float confidenceMean = confidenceSum / tokenConfidences.size();
		
		std::string origWord = cpp->sanitizeWord(currWord);
		std::string replacedWord = cpp->replaceWord(origWord);
		bool spellResult = hpp->spelledCorrectly(replacedWord);
		
		std::unique_ptr<RecognizedWord> word = std::make_unique<RecognizedWord>(
			origWord, replacedWord,
			duration, relStart, relEnd, 
			confidenceMean, spellResult);
		words.push_back(word);
	}
	
	wordMutex.unlock();
	
	tokens.clear();
	
	tokenMutex.unlock();
}

//////////////////////////////////////////////
//
// return string variants:
//
// no detailed result:
//
// { "partial" : "my partial recognition" }
//
// with detailed result:
// 
// { "partial" : "my partial recognition", "listen" : "false" }
// { "partial" : "my partial recognition", "listen" : "true" }
//
//////////////////////////////////////////////
const char* VoskRecognizer::getPartialResult(void)
{
	runTokensToWords();
	
	std::string res = "{ \"partial\" : \"";
	
	wordMutex.lock();
	
	if (words.size() > 0)
	{
		for (unsigned int i = 0; i < words.size(); i++)
		{
			res += words[i]->m_text;
			if (i < (words.size() - 1))
			{
				res += " ";
			}
		}
	}
	
	wordMutex.unlock();
	
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
//
// return string variants:
//
// no detailed result:
//
// { "text" : "my final recognition result" }
//
// with detailed result (old):
// 
// { "text" : "my final recognition result", "start" : "1234567", "startMs" : "345", "stop" : "1234569", "stopMs" : "765"}
//
// with detailed result (new):
// 
// { "text" : "my final recognition result", "start" : "1234567", "startMs" : "345", "stop" : "1234569", "stopMs" : "765",
//   "result": [ { "conf": 1, "end": 1.11, "spell": "true", "start": 0.87, "word": "my"}, { "conf": 0.8, "end": 1.53, "spell": "true", "start": 1.11, "word": "final" } ] }
//
//////////////////////////////////////////////
const char* VoskRecognizer::getFinalResult(void)
{
	std::string res = "{ \"text\" : \"-- ";
    int64_t uStartTime = 0;
    int64_t uStartTimeMs = 0;
    int64_t uStopTime = 0;
    int64_t uStopTimeMs = 0;
	
    utteranceMutex.lock();
    
	if (utterances.size() > 0)
	{
		std::unique_ptr<RecognizedUtterance> fin = std::move(utterances.front());
		utterances.pop_front();
		res += fin->getTotalUtterance();
		
		uStartTime   = fin->m_uStartTime;
		uStartTimeMs = fin->m_uStartTimeMs;
		uStopTime    = fin->m_uStopTime;
		uStopTimeMs  = fin->m_uStopTimeMs;
		
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
			res += "\" ";
			
			// word-level results
			res += ", \"result\": [ ";
			for (unsigned int i = 0; i < fin->getNumberWords(); i++)
			{
				if (i > 0)
				{
					res += ", ";	
				}
				std::unique_ptr<RecognizedWord> word = fin->popWord(i);
				res += "{ \"conf\": "  + std::to_string(word->m_meanConfidence)   + ", ";
				res +=  " \"end\": "   + std::to_string(word->m_relEnd.count())   + ", ";
				res +=  " \"spell\": " + std::to_string(word->m_correctSpelling)  + ", ";
				res +=  " \"start\": " + std::to_string(word->m_relStart.count()) + ", ";
				res +=  " \"word\": "  + word->m_replacer                         + " } ";
			}
			res += "] }";
		}
	}
		
    utteranceMutex.unlock();
    
	std::cout << "Final result: " << res << std::endl;
	
	memset(finalResultBuffer, 0, sizeof(finalResultBuffer));
	strncpy(finalResultBuffer, res.c_str(), sizeof(finalResultBuffer) - 1);
	
	return finalResultBuffer;	
}

//////////////////////////////////////////////
std::unique_ptr<RecognizedUtterance> VoskRecognizer::getFinalResultData(void)
{
	std::unique_ptr<RecognizedUtterance> res = std::make_unique<RecognizedUtterance>();
	
    utteranceMutex.lock();
    
	if (utterances.size() > 0)
	{
		res = std::move(utterances.front());
		utterances.pop_front();
	}
	
    utteranceMutex.unlock();
    
	return res;
}

//////////////////////////////////////////////
int VoskRecognizer::getFrameResolution(void)
{
	return vad->getFrameTimeMs();
}

//////////////////////////////////////////////
void VoskRecognizer::promoteToFinalResult(std::unique_ptr<VADFrameTiming> currStart, std::unique_ptr<VADFrameTiming> currStop)
{
	std::string finalResult;
	float confidence = 0.0f;
	
	runTokensToWords();
	
	wordMutex.lock();
	
	if (words.size() > 0)
	{
		std::unique_ptr<RecognizedUtterance> utt = std::make_unique<RecognizedUtterance>(
			currStart->frameCounter, currStop->frameCounter, 
			currStart->timeStampSeconds, currStart->timeStampMilliSeconds,
			currStop->timeStampSeconds, currStop->timeStampMilliSeconds,
			getFrameResolution());
			
		for (unsigned int i = 0; i < words.size(); i++)
		{
			utt.addWord(words[i]);	
		}
		
		std::cout << "Promoting partial result to final: " << finalResult << ", confidence = " << confidence << std::endl;
		
		audioLogger->flush(utt.getTotalUtterance());
		
		utteranceMutex.lock();
		utterances.push_back(std::move(res));
		utteranceMutex.unlock();
		
		words.clear();
	}
	
	wordMutex.unlock();
}

// #define MEASURE_WHISPER_TIME

//////////////////////////////////////////////
void VoskRecognizer::runWhisper(struct whisper_context* ctx)
{
	// run whisper on the current state of audio buffer
	whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

	wparams.strategy         = WHISPER_SAMPLING_GREEDY;
	
    wparams.print_realtime   = false;
	wparams.print_progress   = false;
	wparams.print_timestamps = env_whisper_no_timestamps; // !default_params.no_timestamps;
	wparams.print_special    = default_params.print_special;
	wparams.translate        = default_params.translate;
	if (env_vosk_model_language == "auto")
	{
		wparams.language         = default_params.language.c_str();
	}
	else
	{
		wparams.language         = env_vosk_model_language.c_str();
	}
    wparams.detect_language  = default_params.detect_language;
    wparams.n_threads        = default_params.n_threads;
    wparams.n_max_text_ctx   = default_params.max_context >= 0 ? default_params.max_context : wparams.n_max_text_ctx;
	if (env_whisper_max_context != -1)
	{
		wparams.n_max_text_ctx = env_whisper_max_context;
	}
    wparams.offset_ms        = default_params.offset_t_ms;
    wparams.duration_ms      = default_params.duration_ms;

    wparams.token_timestamps = default_params.output_wts || default_params.output_jsn_full || default_params.max_len > 0;
    wparams.thold_pt         = default_params.word_thold;
    wparams.max_len          = default_params.output_wts && default_params.max_len == 0 ? 60 : default_params.max_len;
    wparams.split_on_word    = default_params.split_on_word;
    wparams.audio_ctx        = default_params.audio_ctx;

    wparams.debug_mode       = default_params.debug_mode;

    wparams.tdrz_enable      = default_params.tinydiarize; // [TDRZ]

    wparams.suppress_regex   = default_params.suppress_regex.empty() ? nullptr : default_params.suppress_regex.c_str();

    wparams.initial_prompt   = default_params.prompt.c_str();

    wparams.greedy.best_of        = default_params.best_of;
    wparams.beam_search.beam_size = default_params.beam_size;

    wparams.temperature_inc  = env_whisper_no_fallback ? 0.0f : default_params.temperature_inc;
    wparams.temperature      = default_params.temperature;

    wparams.entropy_thold    = default_params.entropy_thold;
    wparams.logprob_thold    = default_params.logprob_thold;

    wparams.no_timestamps    = default_params.no_timestamps;
	    
	// need minimum audio length
	if (pcmf32.size() < pcm_buffer_min)
	{
		pcmf32.insert(pcmf32.cend(), pcm_buffer_min - pcmf32.size(), 0.0f);
	}
	
	// we have a valid instance --> run recognition
	if (ctx)
	{
		tokenMutex.lock();
		
		tokens.clear();
		
		std::cout << "Push audio to whisper, size=" << pcmf32.size() << std::endl;
		
#ifdef MEASURE_WHISPER_TIME
		auto start = std::chrono::high_resolution_clock::now();
#endif

		// this can degrade accuracy if n_processors > 1
		int whisper_call_result = whisper_full_parallel(ctx, wparams, pcmf32.data(), pcmf32.size(), default_params.n_processors);
		
#ifdef MEASURE_WHISPER_TIME
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		std::cout << "whisper call took "	
              << duration.count()
              << " milliseconds\n";
#endif
		
		if (whisper_call_result != 0) 
		{
			// announce the error instead of crashing
			std::string errorText = getLocalTimeStamp().append(": Zmylk při spóznawanju. Spytajće prošu pozdźišo hišće raz.");
			// const char * text = "Zmylk při spóznawanju. Spytajće prošu pozdźišo hišće raz.";
			
			// TBD rather push an utterance than a token???
			std::unique_ptr<RecognizedToken> newResult = std::make_unique<RecognizedToken>(const_cast<char*>(errorText.c_str()), 5000, 200, 4800, 1.0f);
			tokens.push_back(std::move(newResult));
		}
		else
		{
			const int n_segments = whisper_full_n_segments(ctx);
			for (int i = 0; i < n_segments; ++i) {
				const char * text = whisper_full_get_segment_text(ctx, i);
				int64_t t0 = 0;
				int64_t t1 = 0;
		
				// timestamps currently unused anyway?
				if (env_whisper_no_timestamps == false)
				{
					t0 = whisper_full_get_segment_t0(ctx, i);
					t1 = whisper_full_get_segment_t1(ctx, i);
				}
				
				// std::vector<float> tokenProbs;
				const int n_tokens = whisper_full_n_tokens(ctx, i);
				// fprintf(stderr,"tokens: %d\n",n_tokens);
				for (int j = 0; j < n_tokens; j++) {
					auto token = std::string(whisper_full_get_token_text(ctx, i, j));
					float probability = whisper_full_get_token_p(ctx, i, j);
					// std::cout << token << '\t' << probability << std::endl;
					// fprintf(stderr,"token: %s %f\n",token,probability);
					
					// do not use probs from empty tokens and special tokens
					if (!token.empty() && token.front() != '[' && token.back() != ']')
					{
						// just collect all tokens
						std::unique_ptr<RecognizedToken> token = std::make_unique<RecognizedToken>(const_cast<char*>(token), 1000, 200, 800, probability);
						tokens.push_back(std::move(token));
						// tokenProbs.push_back(probability);
					}
					else
					{
						// std::cout << "Excluding token " << token << " from confidence!" << std::endl;	
					}
				}
				
				// TODO could eventually be used for confidence as well
				// float noSpeech = whisper_full_get_segment_no_speech_prob(ctx, i);
				// std::cout << "Segment " << i << '\t' << noSpeech << " no speech prob." << std::endl;
				
				// Compute mean
				
				/*
				float probSum = 0.0f;
				for (float val : tokenProbs) {
					probSum += val;
				}
				float probMean = probSum / tokenProbs.size();
				*/

				// Compute standard deviation
				/*
				float varianceSum = 0.0f;
				for (float val : tokenProbs) {
					varianceSum += (val - probMean) * (val - probMean);
				}
				float stddev = std::sqrt(varianceSum / tokenProbs.size()); // Population std dev
				*/
				
				// std::cout << "Sequence confidence: Mean = " << probMean << ", stddev = " << stddev << "." << std::endl;
				
				// std::unique_ptr<RecognitionResult> newResult = std::make_unique<RecognitionResult>(const_cast<char*>(text), (unsigned int) t0, (unsigned int) t1, (probMean - stddev));
				// partialResult.push_back(std::move(newResult));
			}
		}
		
		tokenMutex.unlock();
	}
	else
	{
		tokenMutex.lock();
		
		tokens.clear();
		
		// supply a dummy result
		std::string errorText = (getLocalTimeStamp().append(": System je přećežene. Spytajće prošu pozdźišo hišće raz."));
		// const char * text = "System je přećežene. Spytajće prošu pozdźišo hišće raz.";
		
		// TBD rather push an utterance than a token???
		std::unique_ptr<RecognizedToken> newResult = std::make_unique<RecognizedToken>(const_cast<char*>(errorText.c_str()), 5000, 200, 4800, 1.0f);
		tokens.push_back(std::move(newResult));
		
		tokenMutex.unlock();
	}
}
