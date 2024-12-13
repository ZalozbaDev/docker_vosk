#ifndef WORD_CLASS_POSTPROC
#define WORD_CLASS_POSTPROC

#include <string>
#include <regex>

struct wc_substr
{
	bool valid;
	size_t beginTag;
	size_t beginExpr;
	size_t endExpr;
	size_t endTag;
};

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
	
	// regex for any word class match in XML style
	std::regex wordClassRegexBegin = std::regex("<[A-Z]*>");

	std::unique_ptr<wc_substr> findTags(std::string line, std::string tagName);
	
	std::string evalMathExpr(std::string input, int precision);
	
	std::string replaceWordClass(std::string line, std::unique_ptr<wc_substr> substr, std::string replacer);
};

#endif // WORD_CLASS_POSTPROC
