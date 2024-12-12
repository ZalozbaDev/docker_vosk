#ifndef WORD_CLASS_POSTPROC
#define WORD_CLASS_POSTPROC

#include <string>
#include <regex>

//////////////////////////////////////////////
class WordClassPostProc
{
public:
	WordClassPostProc(bool active = false);
	~WordClassPostProc(void);
	std::string processLine(std::string line);
private:
	bool passThrough;
	
//	std::regex unwantedChars = std::regex(",|\\.|;|:|\\!|\\?|-");
	
//	std::regex severalSpaces = std::regex("[' ']{2,}");
	
};

#endif // WORD_CLASS_POSTPROC
