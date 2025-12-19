#ifndef CUSTOM_POSTPROC
#define CUSTOM_POSTPROC

#include <string>
#include <regex>

//////////////////////////////////////////////
class CustomPostProc
{
public:
	CustomPostProc(bool active, std::string replacementFile, bool convertCase = false, int maxCharsPerSecond = -1);
	~CustomPostProc(void);
	// legacy all-in-one function
	std::string processLine(std::string line, int lengthInSeconds = -1);
	
	// split functions
	
	std::string sanitizeWord(std::string word);
	int limitLine(std::string line, int lengthInSeconds);
	std::string replaceWord(std::string word);
private:
	bool readReplacementFile(std::string filename);
	std::string replace_all(std::string line, std::string replacee, std::string replacer, std::size_t maxSuffix); 
	
	bool passThrough;
	
	std::regex unwantedChars = std::regex(",|\\.|;|:|\\!|\\?|-");
	
	std::regex severalSpaces = std::regex("[' ']{2,}");
	
	std::regex oneOrMoreSpaces = std::regex("[' ']{1,}");
	
	bool listReplace;
	
	std::vector<std::string> replacees;
	std::vector<std::string> replacers;
	std::vector<std::size_t> maxsuffixes;
	
	int limitCharsPerSecond;
	
	bool convCase;
};

#endif // CUSTOM_POSTPROC
