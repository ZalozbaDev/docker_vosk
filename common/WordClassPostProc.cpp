
#include <iostream>
#include <fstream>

#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/locid.h>

#include <WordClassPostProc.h>

//////////////////////////////////////////////
WordClassPostProc::WordClassPostProc(bool active)
{
	passThrough = !active;
}

//////////////////////////////////////////////
WordClassPostProc::~WordClassPostProc(void)
{

}

//////////////////////////////////////////////
std::string WordClassPostProc::processLine(std::string line)
{
	if (passThrough == true)
	{
		return line;	
	}
	
	std::string retVal = line;
	
	// remove all unwanted symbols
//	std::string retVal = std::regex_replace(line, unwantedChars, " ");
	
	// trim whitespaces at start and end
//	retVal.erase(0, retVal.find_first_not_of(" \n\r\t"));                                                                                               
//	retVal.erase(retVal.find_last_not_of(" \n\r\t")+1);   
	
	// replace several whitespaces by one
//	retVal = std::regex_replace(retVal, severalSpaces, " ");
	
	std::cout << "Orig: '" << line << "' changed to '" << retVal << "'" << std::endl;
	
	return retVal;
}
