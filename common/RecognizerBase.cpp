
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

