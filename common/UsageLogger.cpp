
#include <UsageLogger.h>

std::string UsageLogger::m_path;

///////////////////////////////////////////////////
UsageLogger::UsageLogger(void)
{

}

///////////////////////////////////////////////////
UsageLogger::~UsageLogger(void)
{

}

///////////////////////////////////////////////////
void UsageLogger::setLogPath(std::string path)
{
    m_path = path;
}

///////////////////////////////////////////////////
void UsageLogger::logInstanceNew(int instanceId)
{

}

///////////////////////////////////////////////////
void UsageLogger::logInstanceDelete(int instanceId)
{

}

///////////////////////////////////////////////////
void UsageLogger::logAudioData(int instanceId, int newLength, uint64_t totalLength)
{

}

///////////////////////////////////////////////////
void UsageLogger::logAudioDrop(int instanceId, int dropped)
{

}

///////////////////////////////////////////////////
void UsageLogger::logFinalUtteranceTiming(int64_t uStartTime, int64_t uStartTimeMs, int64_t uStopTime, int64_t uStopTimeMs)
{

}

