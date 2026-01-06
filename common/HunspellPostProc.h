#ifndef HUNSPELL_POSTPROC
#define HUNSPELL_POSTPROC

#include <string>

#include <../hunspell/hunspell.hxx>

//////////////////////////////////////////////
class HunspellPostProc
{
public:
	HunspellPostProc(std::string aff, std::string dic);
	~HunspellPostProc(void);
	std::string processLine(std::string line);
	bool spelledCorrectly(std::string word);
private:
	bool passThrough;
	Hunspell *hsp;
};

#endif // HUNSPELL_POSTPROC
