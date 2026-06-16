
#include "RecognizerBase.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include <VADWrapperWebRTC.h>
#include <VADWrapperSilero.h>
#include <ResamplerWebRTC_48_16.h>
#include <ResamplerWebRTC_8_16.h>
#include <ResamplerLibResample_48_16.h>

int RecognizerBase::voskRecognizerInstanceId = 1;

//////////////////////////////////////////////////////////////////////////////
RecognizerBase::RecognizerBase(int modelId, float sample_rate, const char *configPath, int aggressiveness, const ssize_t processingSampleRate)
{
	std::cout << "vosk_recognizer_new, instance=" << voskRecognizerInstanceId << " sample_rate=" << sample_rate << std::endl;

	m_modelInstanceId = modelId;
	m_instanceId      = voskRecognizerInstanceId++;
	m_inputSampleRate = sample_rate;
	
	
	detailedResults = false;
	
	m_recoState = VoskRecognizerState::UNINIT;
	m_configPath = std::string(configPath);
	
	// safe defaults
	m_isULawSampleFormat = false;
	m_audioChunkLength = 48000;
	m_minNumberAudioPackages = 1;
	
	audioLogger = new AudioLogger(std::string("logs/"), m_instanceId);
    
    if (const char *env_p = std::getenv("VOSK_LOG_AUDIO"))
    {
        if (strcasecmp(env_p, "True") == 0)
        {
        	audioLogger->activate();	
        }
    }
    
    std::string hunspell_aff_file = "";
    if (const char *env_p = std::getenv("VOSK_HUNSPELL_AFF_FILE"))
    {
    	hunspell_aff_file = env_p;
    }
    std::string hunspell_dic_file = "";
    if (const char *env_p = std::getenv("VOSK_HUNSPELL_DIC_FILE"))
    {
    	hunspell_dic_file = env_p;
    }
    hpp = new HunspellPostProc(hunspell_aff_file, hunspell_dic_file);

    std::string replacement_file = "";
    if (const char *env_p = std::getenv("VOSK_REPLACEMENT_FILE"))
    {
    	replacement_file = env_p;
    }
    cpp = new CustomPostProc(true, replacement_file, false, 30); // limit to max. 30 characters per second of audio, reduces impact of hallucinations
    
    // makes sense to tie the resampler to the VAD algo used - not all combinations are possible anyway
    if (const char *env_p = std::getenv("VOSK_VAD_ALGO"))
    {
        if (strcasecmp(env_p, "Silero") == 0)
        {
        	std::cout << "ENV specified, setting VAD algo to Silero." << std::endl;
        	resample = new ResamplerLibResample_48_16();
        	// TBD libresample impl of phone quality to 16kHz
        	vad = new VADWrapperSilero(16000, "model/silero_vad.onnx");
        }
        else
        {
        	std::cout << "ENV specified, setting VAD algo to WebRTC." << std::endl;
        	resample = new ResamplerWebRTC_48_16();
        	resamplePhone = new ResamplerWebRTC_8_16();
        	vad = new VADWrapperWebRTC(aggressiveness, processingSampleRate, 5, 5, 15, 5);
        }
    }
    else
    {
       	std::cout << "ENV empty, setting VAD algo to WebRTC." << std::endl;
       	resample = new ResamplerWebRTC_48_16();
        resamplePhone = new ResamplerWebRTC_8_16();
    	vad = new VADWrapperWebRTC(aggressiveness, processingSampleRate, 5, 5, 15, 5);	
    }
    
    m_probThreshold = -1000.0f;
    if (const char *env_p = std::getenv("VOSK_PROB_THRESHOLD"))
    {
    	m_probThreshold = std::atof(env_p);
    }    
    std::cout << "ENV setting avg prob threshold to  " << m_probThreshold << "." << std::endl;
    
    m_logprobThreshold = -1000.0f;
    if (const char *env_p = std::getenv("VOSK_LOGPROB_THRESHOLD"))
    {
    	m_logprobThreshold = std::atof(env_p);
    }    
    std::cout << "ENV setting avg logprob threshold to  " << m_logprobThreshold << "." << std::endl;
    
    m_rejectResponse = "";
    if (const char *env_p = std::getenv("VOSK_REJECT_RESPONSE"))
    {
    	m_rejectResponse = env_p;
    }
    if (m_rejectResponse.length() > 0)
    {
    	std::cout << "ENV setting reject response to  " << m_rejectResponse << "." << std::endl;
    }
    else
    {
    	std::cout << "ENV setting NO reject response." << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////
RecognizerBase::~RecognizerBase()
{
	// now we can free all resources
	delete(cpp);
	delete(hpp);
	
	delete(audioLogger);
	
	delete(vad);
	delete(resamplePhone);
	delete(resample);

	tokens.clear();
	words.clear();
	utterances.clear();
	
	std::cout << "vosk_recognizer_free, instance=" << m_instanceId << std::endl;

	// don't decrease, let every instance get a unique ID
	// voskRecognizerInstanceId--;
}

//////////////////////////////////////////////////////////////////////////////
std::string RecognizerBase::getLocalTimeStamp()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto str = oss.str();

    return str;
}

//////////////////////////////////////////////////////////////////////////////
void RecognizerBase::setDetailedResult(bool detailsOn)
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

//////////////////////////////////////////////////////////////////////////////
void RecognizerBase::setSampleRate(float rate)
{
	m_inputSampleRate = rate;
	std::cout << "RecognizerBase::setSampleRate=" << m_inputSampleRate << std::endl;
	recomputeMinNumberAudioPackages();
}

//////////////////////////////////////////////////////////////////////////////
void RecognizerBase::setSampleFormat(const char *format)
{
	std::string tmpFormat(format);
	m_isULawSampleFormat = (tmpFormat == "ULAW") ? true : false;
	std::cout << "RecognizerBase::setSampleFormat ULAW=" << m_isULawSampleFormat << std::endl;
	recomputeMinNumberAudioPackages();
}

//////////////////////////////////////////////////////////////////////////////
void RecognizerBase::setChunklen(int length)
{
	m_audioChunkLength = length;
	std::cout << "RecognizerBase::setChunklen=" << m_audioChunkLength << std::endl;
	recomputeMinNumberAudioPackages();
}

//////////////////////////////////////////////////////////////////////////////
void RecognizerBase::recomputeMinNumberAudioPackages(void)
{
	// how many packages need to be collected before audio processing
	// to not break our (broken) VAD algorithm?
	// TBD this can be removed once the algo is fixed
	
	
	
	float tmpAudioChunkLen = (float) m_audioChunkLength;
	// take care of different sample sizes
	if (m_isULawSampleFormat == false)
	{
		// buffer only contains half the samples at 16 bit
		tmpAudioChunkLen = tmpAudioChunkLen / 2;
	}
	
	float packetsPerSecond = m_inputSampleRate / tmpAudioChunkLen;
	
	// assure at least 80ms of audio to be collected before processing
	// which is equal to 12,5 packets / second
	if (packetsPerSecond < 12.5f)
	{
		m_minNumberAudioPackages = 1;	
	}
	else
	{
		// accumulate packets to meet the 80ms goal
		//
		// 48kHz PCM16SE with packets=4096 --> 2 packets --> 85ms  audio 
		// 8kHz ULAW with packets=160      --> 5 packets --> 100ms audio

		m_minNumberAudioPackages = ((int) (packetsPerSecond / 12.5f)) + 1;
	}
	
	std::cout << "RecognizerBase::recomputeMinNumberAudioPackages = " << m_minNumberAudioPackages << std::endl;
}

//////////////////////////////////////////////
void RecognizerBase::setTimeStamp(int64_t seconds, int64_t uSeconds)
{
	// std::cout << "TIMESTAMP: " << seconds << "." << uSeconds << "s" << std::endl;
	clientTimeStamp = std::chrono::system_clock::from_time_t(seconds) + std::chrono::microseconds(uSeconds);
	auto timeStampPrint = std::chrono::system_clock::to_time_t(clientTimeStamp);
	std::cout << "TIMESTAMP: " << std::ctime(&timeStampPrint) << std::endl;
}

//////////////////////////////////////////////
bool RecognizerBase::getRecognizerBusy(bool audioQueueOnly)
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
int RecognizerBase::acceptWaveform(const char *data, int length)
{
	int retVal;
	bool validSampleConfig = true;
	
	// verify allowed combinations of sample rate & sample format
	if (getProcessingSampleRate() != 16000) validSampleConfig = false;
	if (((m_inputSampleRate == 48000) || (m_inputSampleRate == 16000)) && (m_isULawSampleFormat == true)) validSampleConfig = false;
	if ((m_inputSampleRate == 8000) && (m_isULawSampleFormat == false)) validSampleConfig = false;
	
	// supported, store data and notify consumer
	if (validSampleConfig == true)
	{
	
		// create object and copy all data
		std::unique_ptr packet = std::make_unique<AudioPacket>();
		packet->length      = length;
		packet->data        = new char[length];
		packet->arrivalTime = std::chrono::system_clock::now();
		memcpy(packet->data, data, length);
	
#if 0	
	
		auto now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration)
							- std::chrono::duration_cast<std::chrono::milliseconds>(seconds);
	
		std::cout << "acceptWaveform push len=" << length << ", time=" << seconds.count() << "." << milliseconds.count() << std::endl;
	
#endif
			
		// push to queue and notify worker
		std::unique_lock<std::mutex> audioPacketLock{audioPacketMutex};
		audioPackets.push_back(std::move(packet));
		audioPacketLock.unlock();
		audioPacketNotify.notify_one();

	}
	else
	{
		std::cout << "Error! Unsupported combination of sample rate " << m_inputSampleRate 
		          << "Hz and sample size " << ((m_isULawSampleFormat == true) ? "8" : "16") << "bit!"
		          << std::endl;
	}
	
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
bool RecognizerBase::getPartialStatus(void)
{
	return ((vad->getUtteranceStatus() != VADWrapperState::IDLE) ? true : false);
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
const char* RecognizerBase::getPartialResult(void)
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
	
	// std::cout << "Partial result: " << res << std::endl;
	
	memset(partialResultBuffer, 0, sizeof(partialResultBuffer));
	strncpy(partialResultBuffer, res.c_str(), sizeof(partialResultBuffer) - 1);
	
	return partialResultBuffer;	
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
//   "result": [ { "conf": "1", "end": "1.11", "spell": "true", "start": "0.87", "word": "my"}, 
//               { "conf": "0.8", "end": ""1.53"", "spell": "true", "start": "1.11", "word": "final" } ] }
//
//////////////////////////////////////////////
const char* RecognizerBase::getFinalResult(void)
{
	std::string res = "{ \"text\" : \"";
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
			res += "\" }";
		}
		else
		{
			res += "\", \"start\" : \"";
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
				res += "{ \"conf\": \""  + std::to_string(word->m_meanConfidence)   + "\", ";
				res +=  " \"end\": \""   + std::to_string(word->m_relEnd.count())   + "\", ";
				res +=  " \"spell\": \"" + std::to_string(word->m_correctSpelling)  + "\", ";
				res +=  " \"start\": \"" + std::to_string(word->m_relStart.count()) + "\", ";
				res +=  " \"word\": \""  + word->m_replacer                         + "\" } ";
			}
			res += "] }";
		}
	}
	else
	{
		res += "\" }";
	}
		
    utteranceMutex.unlock();
    
	std::cout << "Final result: " << res << std::endl;
	
	// FIXME shall log if text would not fit buffer!
	memset(finalResultBuffer, 0, sizeof(finalResultBuffer));
	strncpy(finalResultBuffer, res.c_str(), sizeof(finalResultBuffer) - 1);
	
	return finalResultBuffer;	
}

//////////////////////////////////////////////
std::unique_ptr<RecognizedUtterance> RecognizerBase::getFinalResultData(void)
{
	std::unique_ptr<RecognizedUtterance> res = std::make_unique<RecognizedUtterance>(0, 10, 0, 0, 0, 100, vad->getFrameTimeMs(), cpp);
	
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
int RecognizerBase::getFrameResolution(void)
{
	return vad->getFrameTimeMs();
}

//////////////////////////////////////////////
void RecognizerBase::promoteToFinalResult(std::unique_ptr<VADFrameTiming> currStart, std::unique_ptr<VADFrameTiming> currStop)
{
	runTokensToWords();
	
	wordMutex.lock();
	
	if (words.size() > 0)
	{
		float confidence = 0.0f;
		double avgLogProb = 0.0f;
	
		std::unique_ptr<RecognizedUtterance> utt = std::make_unique<RecognizedUtterance>(
			currStart->frameCounter, currStop->frameCounter, 
			currStart->timeStampSeconds, currStart->timeStampMilliSeconds,
			currStop->timeStampSeconds, currStop->timeStampMilliSeconds,
			getFrameResolution(), cpp);
			
		for (unsigned int i = 0; i < words.size(); i++)
		{
			utt->addWord(std::move(words[i]));	
		}
		
		confidence = utt->getTotalConfidenceMean();
		avgLogProb = utt->getAvgLogProb();
		
		audioLogger->flush(utt->getTotalUtterance());
		
		// reject/discard utterance if either
		// avg_prob (0 .. 1) < threshold (e.g. 0.85)
		// or
		// avg_logprob (-x.y .. 0) < threshold (e.g. -1.0)
		//
		// default thresholds shall avoid any rejection
		if ((confidence < m_probThreshold) || (avgLogProb < m_logprobThreshold))
		{
			std::cout << "DISCARD partial result '" << utt->getTotalUtterance() << "', avg_prob=" << confidence 
			          << ", avg_logProb=" << avgLogProb << std::endl;
			          
			// only if a response for rejections is set
			if (m_rejectResponse.length() > 0)
			{
				utt->resetWords();
				std::unique_ptr<RecognizedWord> word = std::make_unique<RecognizedWord>(
					(char*) m_rejectResponse.c_str(), (char*) m_rejectResponse.c_str(), std::chrono::milliseconds(1000), 
					std::chrono::milliseconds(100), std::chrono::milliseconds(900),
					confidence, true, avgLogProb);
				utt->addWord(std::move(word));
				// force sanitize again
				(void) utt->getNumberWords();
				
				utteranceMutex.lock();
				utterances.push_back(std::move(utt));
				utteranceMutex.unlock();
			}
		}
		else
		{
			std::cout << "Promoting partial result to final, avg_prob=" << confidence 
			          << ", logProb=" << avgLogProb << std::endl;
			utteranceMutex.lock();
			utterances.push_back(std::move(utt));
			utteranceMutex.unlock();
		}
		
		words.clear();
	}
	
	wordMutex.unlock();
}
