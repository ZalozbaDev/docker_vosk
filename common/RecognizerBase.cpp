
#include "RecognizerBase.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include <VADWrapperWebRTC.h>
#include <VADWrapperSilero.h>
#include <ResamplerWebRTC_48_16.h>
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
        	std::cout << "ENV setting VAD algo to Silero." << std::endl;
        	resample = new ResamplerLibResample_48_16();
        	vad = new VADWrapperSilero(16000, "model/silero_vad.onnx");
        }
        else
        {
        	std::cout << "ENV setting VAD algo to WebRTC." << std::endl;
        	resample = new ResamplerWebRTC_48_16();
        	vad = new VADWrapperWebRTC(aggressiveness, processingSampleRate, 5, 5, 5, 5);
        }
    }
    else
    {
       	std::cout << "ENV setting VAD algo to WebRTC." << std::endl;
       	resample = new ResamplerWebRTC_48_16();
    	vad = new VADWrapperWebRTC(aggressiveness, processingSampleRate, 5, 5, 5, 5);	
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
	delete(resample);

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
	
