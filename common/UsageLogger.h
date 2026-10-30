#ifndef USAGE_LOGGER_H
#define USAGE_LOGGER_H

#include <deque>
#include <memory>
#include <string>


//////////////////////////////////////////////
class UsageLogger
{
public:
    static void setLogPath(std::string path);
    static void logInstanceNew(int instanceId);
    static void logInstanceDelete(int instanceId);
    static void logAudioData(int instanceId, int newLength, uint64_t totalLength);
    static void logAudioDrop(int instanceId, int dropped);
    static void logFinalUtteranceTiming(int64_t uStartTime, int64_t uStartTimeMs, int64_t uStopTime, int64_t uStopTimeMs);

	~UsageLogger(void);
private:
	UsageLogger(void);

    static std::string m_path;
};

#endif // USAGE_LOGGER_H
