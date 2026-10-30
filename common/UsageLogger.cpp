
#include <iostream>

#include <UsageLogger.h>

std::string UsageLogger::m_path;
std::vector<std::unique_ptr<UsageStats>> UsageLogger::m_stats;

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
    std::unique_ptr<UsageStats> stat = std::make_unique<UsageStats>();
    stat->instanceId = instanceId;
    m_stats.push_back(std::move(stat));
}

///////////////////////////////////////////////////
void UsageLogger::logInstanceDelete(int instanceId)
{

}

///////////////////////////////////////////////////
void UsageLogger::logAudioData(int instanceId, int newLength, uint64_t totalLength)
{
    int index = getStatsIndex(instanceId);

    if (index > -1)
    {
        m_stats[index]->nrAudioPackets++;
        m_stats[index]->maxAudioQueueLength = std::max(m_stats[index]->maxAudioQueueLength, totalLength);
    } 
    else
    {
        std::cout << "UsageLogger::logAudioData Error - cannot find instance " << instanceId << "." << std::endl;
    } 
}

///////////////////////////////////////////////////
void UsageLogger::logAudioDrop(int instanceId, int dropped)
{
    int index = getStatsIndex(instanceId);

    if (index > -1)
    {
        m_stats[index]->droppedDataLength += dropped;
    } 
    else
    {
        std::cout << "UsageLogger::logAudioDrop Error - cannot find instance " << instanceId << "." << std::endl;
    } 
}

///////////////////////////////////////////////////
void UsageLogger::logFinalUtteranceTiming(int instanceId, int64_t uStartTime, int64_t uStartTimeMs, int64_t uStopTime, int64_t uStopTimeMs)
{
    int index = getStatsIndex(instanceId);

    if (index > -1)
    {
        m_stats[index]->nrRecognizedUtterances++;
    } 
    else
    {
        std::cout << "UsageLogger::logFinalUtteranceTiming Error - cannot find instance " << instanceId << "." << std::endl;
    } 
}

///////////////////////////////////////////////////
int UsageLogger::getStatsIndex(int instanceId)
{
    for (unsigned int i = 0; i < m_stats.size(); i++)
    {
        if (m_stats[i]->instanceId == instanceId)
        {
            return i;
        } 
    } 

    return -1;
}
