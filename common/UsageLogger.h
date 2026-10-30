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

	~UsageLogger(void);
private:
	UsageLogger(void);

    static std::string m_path;
};

#endif // USAGE_LOGGER_H
