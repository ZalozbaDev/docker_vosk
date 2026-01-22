
#include "RecognizerBase.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

int RecognizerBase::voskRecognizerInstanceId = 1;

//////////////////////////////////////////////////////////////////////////////
RecognizerBase::RecognizerBase(int modelId, float sample_rate, const char *configPath, int aggressiveness)
{
	std::cout << "vosk_recognizer_new, instance=" << voskRecognizerInstanceId << " sample_rate=" << sample_rate << std::endl;

	m_modelInstanceId = modelId;
	m_instanceId      = voskRecognizerInstanceId++;
	m_inputSampleRate = sample_rate;
	
	detailedResults = false;
	
	

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

