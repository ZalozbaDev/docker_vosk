#ifndef CUSTOM_POSTPROC
#define CUSTOM_POSTPROC

#include <string>
#include <regex>

//////////////////////////////////////////////
class CustomPostProc
{
public:
	CustomPostProc(bool active);
	~CustomPostProc(void);
	std::string processLine(std::string line);
private:
	bool passThrough;
	
	std::regex unwantedChars = std::regex(",|\\.|;|:|\\!|\\?");
	
	std::regex severalSpaces = std::regex("[' ']{2,}");
	
};

#endif // CUSTOM_POSTPROC
