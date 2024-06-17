
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
	
	// remove all unwanted symbols
	std::string retVal = std::regex_replace(line, unwantedChars, " ");
	
	// trim whitespaces at start and end
	retVal.erase(0, retVal.find_first_not_of(" \n\r\t"));                                                                                               
	retVal.erase(retVal.find_last_not_of(" \n\r\t")+1);   
	
	// replace several whitespaces by one
	retVal = std::regex_replace(retVal, severalSpaces, " ");
	
	// std::cout << "Orig: '" << line << "' changed to '" << retVal << "'" << std::endl;
	
	return retVal;
}

