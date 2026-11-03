
#include <iostream>
#include <filesystem>
#include <ios>
#include <fstream>
#include <sstream>

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
    stat->startTimeStamp = getTimeStampString();
    m_stats.push_back(std::move(stat));
}

///////////////////////////////////////////////////
void UsageLogger::logInstanceDelete(int instanceId)
{
    std::string filename;

    int index = getStatsIndex(instanceId);

    if (index > -1)
    {
        // do not write empty statistics (never received any audio)
        if (m_stats[index]->nrAudioPackets > 0)
        { 
            filename = m_path + m_stats[index]->startTimeStamp + "_" + std::to_string(instanceId) + "_" + getTimeStampString() + ".log";

            std::ofstream textStream(filename.c_str(), std::ofstream::out);
            
            if ((textStream.rdstate() & (std::ofstream::failbit | std::ofstream::badbit)) != 0)
            {
                std::cout << "Error opening " << filename << " for writing!" << std::endl;
            }
            else
            {
                textStream << "nrAudioPackets="         << m_stats[index]->nrAudioPackets         << std::endl;
                textStream << "maxAudioQueueLength="    << m_stats[index]->maxAudioQueueLength    << std::endl;
                textStream << "droppedDataLength="      << m_stats[index]->droppedDataLength      << std::endl;
                textStream << "nrRecognizedUtterances=" << m_stats[index]->nrRecognizedUtterances << std::endl;
            
                if (textStream.good() == false)
                {
                    std::cout << "Error writing text file " << filename << std::endl;	
                }
                
                textStream.close();
            }
        }
        else
        {
            std::cout << "UsageLogger::logInstanceDelete - Instance " << instanceId << " was never active." << std::endl;
        }
        
        // finally remove the element from vector
        m_stats.erase(m_stats.begin() + index);
    } 
    else
    {
        std::cout << "UsageLogger::logInstanceDelete Error - cannot find instance " << instanceId << "." << std::endl;
    } 
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

///////////////////////////////////////////////////
std::string UsageLogger::getTimeStampString(void)
{
    std::ostringstream os;
    
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    os << std::put_time(&tm, "%y%m%d_%H%M%S");
    
    return os.str();
}
