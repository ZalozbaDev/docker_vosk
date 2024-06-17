
#include <iostream>

#include <CustomPostProc.h>

//////////////////////////////////////////////
CustomPostProc::CustomPostProc(bool active)
{
	passThrough = !active;
}

//////////////////////////////////////////////
CustomPostProc::~CustomPostProc(void)
{
	
}

//////////////////////////////////////////////
std::string CustomPostProc::processLine(std::string line)
{
	if (passThrough == true)
	{
		return line;	
	}
	
	
}

