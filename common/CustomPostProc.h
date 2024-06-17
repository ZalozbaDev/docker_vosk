#ifndef CUSTOM_POSTPROC
#define CUSTOM_POSTPROC

#include <string>

//////////////////////////////////////////////
class CustomPostProc
{
public:
	CustomPostProc(bool active);
	~CustomPostProc(void);
	std::string processLine(std::string line);
private:
	bool passThrough;
};

#endif // CUSTOM_POSTPROC
