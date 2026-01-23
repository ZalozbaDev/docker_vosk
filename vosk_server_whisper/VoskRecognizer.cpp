
#include <VoskRecognizer.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <cassert>
#include <regex>
#include <chrono>

using namespace std::chrono_literals;

//////////////////////////////////////////////
VoskRecognizer::VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness) : 
RecognizerBase(modelId, sample_rate, configPath, aggressiveness, m_processingSampleRate)
{
	// capture recognizer-specific options from envvars
	int         env_whisper_max_context   = -1; // use -1 for "don't change default"
	bool        env_whisper_no_timestamps = false;
	bool        env_whisper_no_fallback   = false;
	bool        env_whisper_force_cpu     = false;
	std::string env_vosk_model_language   = "auto";

    // optional environment var
    // - --language            ("en", "czech", ...)
    if (const char *env_p = std::getenv("VOSK_MODEL_LANGUAGE"))
    {
    	env_vosk_model_language = env_p;
    }    
    std::cout << "ENV setting language to '" << env_vosk_model_language << "'." << std::endl;
    
    // optional environment var
    // - -mc / --max-context   (a.k.a. "n_max_text_ctx":   default = 16384, some models need this to be 0)
    if (const char *env_p = std::getenv("VOSK_WHISPER_MAX_CONTEXT"))
    {
    	env_whisper_max_context = std::atoi(env_p);
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
    std::cout << "ENV setting whisper no fallback to " << env_whisper_no_fallback << "." << std::endl;
    
    if (const char *env_p = std::getenv("VOSK_WHISPER_USE_CPU"))
    {
        if (strcasecmp(env_p, "True") == 0)
        {
        	env_whisper_force_cpu = true;
        	// default_params.use_gpu = false;
        }
    }
    std::cout << "ENV setting whisper use GPU to  " << env_whisper_force_cpu << "." << std::endl;

	// init whisper impl with all the collected options
	WhisperPool::setWhisperParams(m_configPath, env_vosk_model_language, env_whisper_max_context, 
		env_whisper_no_timestamps, env_whisper_no_fallback, env_whisper_force_cpu);
	WhisperPool::allocate(1);
	
	// temporalily allocate the instance for the announcement string
	std::unique_ptr<WhisperImpl> whisperInst = WhisperPool::getInstance();
	
	// announce the details of the impl
	std::unique_ptr<RecognizedUtterance> res = std::make_unique<RecognizedUtterance>(0, 125, 0, 0, 2, 0, vad->getFrameTimeMs(), cpp);
	std::string voskAnnouncementString = whisperInst->getAnnouncementString();
	std::unique_ptr<RecognizedWord> wrd = std::make_unique<RecognizedWord>((char*) voskAnnouncementString.c_str(), (char*) voskAnnouncementString.c_str(), 2000ms, 100ms, 1900ms, 1.0f, true);
	res->addWord(std::move(wrd));
	utterances.push_back(std::move(res));
	
	WhisperPool::releaseInstance(std::move(whisperInst));
}

//////////////////////////////////////////////
VoskRecognizer::~VoskRecognizer(void)
{
	WhisperPool::unregister();
}

//////////////////////////////////////////////
void VoskRecognizer::workerThreadFunc(void)
{
	bool threadAlive;
	
	int status;
	bool noMoreData;
	
	// splitting audio into chunks & resampling to 16kHz
	// make this depend on the required framelength for VAD
	const int framelen16 = vad->getRequiredFrameLength();
	const int framelen48 = framelen16 * 3;
	int16_t buf[framelen16];
	
	// leftover data buffer should not be bigger than one audio frame
	leftOverData = new char[framelen48 * 2];
	
	pcmf32.clear();
	pcmBufferFragmented = false;
	currFragmentStartTime = std::make_unique<VADFrameTiming>();
		
	m_recoState = VoskRecognizerState::INIT;
	
	std::vector<RecognizedToken> recoTokens;
	
	// temporalily allocate the instance for the "short buffer" value
	std::unique_ptr<WhisperImpl> whisperInst = WhisperPool::getInstance();
	unsigned int shortAudioBufferSizeSamples = whisperInst->getShortAudioBufferSizeSamples();
	unsigned int maxAudioBufferSizeSamples = whisperInst->getMaxAudioBufferSizeSamples();
	WhisperPool::releaseInstance(std::move(whisperInst));

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
			
			noMoreData = vad->analyze((pcmf32.size() < shortAudioBufferSizeSamples) ? true : false);
			
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
				
				if ((detectedUttFinished == true) || (pcmf32.size() > maxAudioBufferSizeSamples))
				{
					
					recoTokens.clear();
					
					std::unique_ptr<WhisperImpl> whisperInst = WhisperPool::getInstance();
					
					whisperInst->run(pcmf32, recoTokens);
					
					WhisperPool::releaseInstance(std::move(whisperInst));
					
					tokenMutex.lock();
					
					// FIXME inefficient!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					
					tokens.reserve(tokens.size() + recoTokens.size());
					for (RecognizedToken t : recoTokens)
					{
						tokens.push_back(std::make_unique<RecognizedToken>(t));
					}

					// tokens.insert(tokens.end(), recoTokens.begin(), recoTokens.end());
					
					tokenMutex.unlock();
					
					recoTokens.clear();
					
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
		
				noMoreData = vad->analyze((pcmf32.size() < shortAudioBufferSizeSamples) ? true : false);
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
				(char*) origWord.c_str(), (char*) replacedWord.c_str(),
				duration, relStart, relEnd, 
				confidenceMean, spellResult);
			words.push_back(std::move(word));
			
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
			(char*) origWord.c_str(), (char*) replacedWord.c_str(),
			duration, relStart, relEnd, 
			confidenceMean, spellResult);
		words.push_back(std::move(word));
	}
	
	wordMutex.unlock();
	
	tokens.clear();
	
	tokenMutex.unlock();
}
