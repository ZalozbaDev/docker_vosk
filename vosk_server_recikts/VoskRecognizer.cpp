
#include <VoskRecognizer.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <cassert>
#include <regex>
#include <chrono>

using namespace std::chrono_literals;

//////////////////////////////////////////////
VoskRecognizer::VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness) : 
RecognizerBase(modelId, sample_rate, configPath, aggressiveness, m_processingSampleRate)
{
	recIktsImpl = new RecIKTSImpl(m_configPath);
	
	// this is now hard-coded in runTokensToWords()
	
	/*	
    if (const char *env_p = std::getenv("VOSK_SUBWORD_REGEX"))
    {
    	subword_regex = std::string(env_p);
    	std::cout << "Using regex '" << subword_regex << "' for subword  merging." << std::endl;
    }
    else
    {
    	subword_regex = std::string("");	
    }
    */

    cpp->setConvertCase(true);
    
    
	std::unique_ptr<RecognizedUtterance> res = std::make_unique<RecognizedUtterance>(0, 250, 0, 0, 3, 0, vad->getFrameTimeMs(), cpp);
	std::string voskAnnouncementString = recIktsImpl->getAnnouncementString();
	std::unique_ptr<RecognizedWord> wrd = std::make_unique<RecognizedWord>((char*) voskAnnouncementString.c_str(), (char*) voskAnnouncementString.c_str(), 2000ms, 100ms, 1900ms, 1.0f, true);
	res->addWord(std::move(wrd));
	utterances.push_back(std::move(res));	
	
    lastUttStopTime = 0;
    checkUtterancePause = false;
    
   	// finally start the recognizer thread
	m_vadFrameCounter = 0;
	
    clientTimeStamp = std::chrono::system_clock::now();
    
    threadRunning = true;    
    recoWorkerThread = new std::thread(&VoskRecognizer::workerThreadFunc, this);
}

//////////////////////////////////////////////
void VoskRecognizer::changeConfigPath(const char *newPath)
{
	std::cout << "VoskRecognizer::changeConfigPath stub!" << std::endl;
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

	// only now we can unregister our instances
	delete(recIktsImpl);
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
	
	m_recoState = VoskRecognizerState::INIT;
	
	std::vector<RecognizedToken> recoTokens;
	
	lastUttStopTime = 0;
	checkUtterancePause = false;
	
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
			
			// splitting audio into chunks & resampling to 16kHz
			while(leftOverDataLen + length >= framelen48 * 2){
		
				int useLen = framelen48 * 2 - leftOverDataLen;
				memcpy(leftOverData + leftOverDataLen, data, useLen);
				data += useLen;
				length -= useLen;
				leftOverDataLen = 0;
		
				resample->resample((const int16_t*)leftOverData, buf, framelen48);
		  
				// TODO we could remove all leftover handling from VAD
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
			
			noMoreData = vad->analyze();
			
			while (noMoreData == false)
			{
				unsigned int availableChunks = vad->getAvailableChunks();
				VADWrapperState uttStatus;
				bool detectedUttFinished = false;
				std::unique_ptr<VADFrameTiming> currStart = std::make_unique<VADFrameTiming>();
				std::unique_ptr<VADFrameTiming> currStop = std::make_unique<VADFrameTiming>();
				
				assert(availableChunks > 0);
				
				// std::cout << "Processing " << availableChunks << " announced chunks." << std::endl;
				
				while (availableChunks > 0)
				{
					uttStatus = vad->getUtteranceStatus();
					
					// get the utterance start and stop properties from VAD wrapper
					if ((currStart->valid == false) && (uttStatus != VADWrapperState::IDLE))
					{
						currStart = vad->getUtteranceStart();
						
						if (checkUtterancePause == true)
						{
							// evaluate the pause between utterances and signal this to the recognizer
							if ((currStart->timeStampSeconds - lastUttStopTime) > longPauseSeconds)
							{
								recIktsImpl->startUtterance(true);	
							}
							else
							{
								recIktsImpl->startUtterance(false);	
							}
							checkUtterancePause = false;	
						}
					}
					
					if ((currStop->valid == false) && (uttStatus == VADWrapperState::POSTBUF))
					{
						currStop = vad->getUtteranceStop();
					}
					
					std::unique_ptr<VADFrame> chunk = vad->getNextChunk();
					
					recIktsImpl->consumeAudio(chunk->samples, chunk->m_numberSamples);
					
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
					// TBD think about when to announce the "restart"
					recIktsImpl->finalizeUtterance();
					
					recIktsImpl->getRecognizedTokens(recoTokens);
					
					tokenMutex.lock();
					
					// FIXME inefficient!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					
					tokens.reserve(tokens.size() + recoTokens.size());
					for (RecognizedToken t : recoTokens)
					{
						tokens.push_back(std::make_unique<RecognizedToken>(t));
					}

					tokenMutex.unlock();
					
					recoTokens.clear();
					
					// as opposed to whisper, where we need to check scenarios with fragmented audio buffers
					// with recikts everything is already processed and we have one scenario only 
					// (not fragmented, utterance finished)
					
					assert(currStart->valid == true);
					assert(currStop->valid == true);

					// save values before invalidating instances
					lastUttStopTime = currStop->timeStampSeconds;
					checkUtterancePause = true;

					promoteToFinalResult(std::move(currStart), std::move(currStop));
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
		
	// promoteToFinalResult();
	
	delete[] leftOverData;

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
	
	std::regex subword_delim("#");
		
	for (auto&& token : tokens)
	{
		// check new word
		if ((token->m_text[0] != '#') && (currWord.length() > 0))
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
		currWord += std::regex_replace(token->m_text, subword_delim, "");
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
