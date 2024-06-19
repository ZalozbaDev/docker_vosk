#ifndef CUSTOM_POSTPROC
#define CUSTOM_POSTPROC

#include <string>
#include <regex>

//////////////////////////////////////////////
class CustomPostProc
{
public:
	CustomPostProc(bool active, std::string replacementFile);
	~CustomPostProc(void);
	std::string processLine(std::string line);
private:
	bool readReplacementFile(std::string filename);
	std::string replace_all(std::string line, std::string replacee, std::string replacer, std::size_t maxSuffix); 
	
	bool passThrough;
	
	std::regex unwantedChars = std::regex(",|\\.|;|:|\\!|\\?");
	
	std::regex severalSpaces = std::regex("[' ']{2,}");
	
	bool listReplace;
	
	std::vector<std::string> replacees;
	std::vector<std::string> replacers;
	std::vector<std::size_t> maxsuffixes;
};

#endif // CUSTOM_POSTPROC
