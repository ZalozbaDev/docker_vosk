#ifndef USAGE_LOGGER_H
#define USAGE_LOGGER_H

#include <memory>
#include <string>
#include <vector>

//////////////////////////////////////////////
class UsageStats
{
public:
    UsageStats() { instanceId = 0; nrAudioPackets = 0; maxAudioQueueLength = 0; droppedDataLength = 0; nrRecognizedUtterances = 0; } 

    int      instanceId;
    uint64_t nrAudioPackets;
    uint64_t maxAudioQueueLength;
    uint64_t droppedDataLength;
    uint64_t nrRecognizedUtterances;
};

//////////////////////////////////////////////
class UsageLogger
{
public:
    static void setLogPath(std::string path);
    static void logInstanceNew(int instanceId);
    static void logInstanceDelete(int instanceId);
    static void logAudioData(int instanceId, int newLength, uint64_t totalLength);
    static void logAudioDrop(int instanceId, int dropped);
    static void logFinalUtteranceTiming(int instanceId, int64_t uStartTime, int64_t uStartTimeMs, int64_t uStopTime, int64_t uStopTimeMs);

	~UsageLogger(void);
private:
	UsageLogger(void);

    static int getStatsIndex(int instanceId);

    static std::string m_path;

    static std::vector<std::unique_ptr<UsageStats>> m_stats;
};

#endif // USAGE_LOGGER_H
