#ifndef HUNSPELL_POSTPROC
#define HUNSPELL_POSTPROC

#include <string>

#include <../hunspell/hunspell.hxx>

//////////////////////////////////////////////
class HunspellPostProc
{
public:
	HunspellPostProc(std::string aff, std::string dic, std::string custom);
	~HunspellPostProc(void);
	std::string processLine(std::string line);
private:
	bool passThrough;
	Hunspell *hsp;
};

#endif // HUNSPELL_POSTPROC
