
#include <VoskRecognizer.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <cassert>
#include <regex>


//////////////////////////////////////////////
VoskRecognizer::VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness) : 
RecognizerBase(modelId, sample_rate, configPath, aggressiveness, m_processingSampleRate)
{
	recIktsImpl = new RecIKTSImpl(m_configPath);
	
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

	std::unique_ptr<RecognizedUtterance> res = std::make_unique<RecognizedUtterance>(0, 125, 0, 0, 2, 0, vad->getFrameTimeMs(), cpp);
	std::string voskAnnouncementString = recIktsImpl->getAnnouncementString();
	std::unique_ptr<RecognizedWord> wrd = std::make_unique<RecognizedWord>((char*) voskAnnouncementString.c_str(), (char*) voskAnnouncementString.c_str(), 2000ms, 100ms, 1900ms, 1.0f, true);
	res->addWord(std::move(wrd));
	utterances.push_back(std::move(res));	
	
    lastUttStopTime = 0;
    longPauseBetweenUtterances = true;
}

//////////////////////////////////////////////
VoskRecognizer::~VoskRecognizer(void)
{
	delete(recIKTSImpl);
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
				
				assert(availableChunks > 0);
				
				// std::cout << "Processing " << availableChunks << " announced chunks." << std::endl;
				
				while (availableChunks > 0)
				{
					uttStatus = vad->getUtteranceStatus();
					
					std::unique_ptr<VADFrame> chunk = vad->getNextChunk();
					
					recIktsInst->consume(chunk->samples, chunk->m_numberSamples);
					
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
					recIKTSInst->flush(longPauseBetweenUtterances);
					
					promoteToFinalResult();
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
	
	delete[] leftOverData;

    // initStatus = recikts_stop();
    // initStatus = cfgikts_free(&recikts_cfg);
	
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
		currWord += std::regex_replace(token->m_text, subword, "");
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
int VoskRecognizer::getFrameResolution(void)
{
	return vad->getFrameTimeMs();
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

