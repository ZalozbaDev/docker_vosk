
#include <iostream>
#include <sstream>

#include <HunspellPostProc.h>

//////////////////////////////////////////////
HunspellPostProc::HunspellPostProc(std::string aff, std::string dic, std::string custom)
{
	if ((aff.length() < 1) || (dic.length() < 1))
	{
		std::cout << "No valid hunspell data --> passthrough mode!" << std::endl;
		passThrough = true;
	}
	else
	{
		std::cout << "Real dictionary mode!" << std::endl;
		hsp = new Hunspell(aff.c_str(), dic.c_str());
		passThrough = false;
	}
}

//////////////////////////////////////////////
HunspellPostProc::~HunspellPostProc(void)
{
	delete(hsp);
}

//////////////////////////////////////////////
std::string HunspellPostProc::processLine(std::string line)
{
	if (passThrough == true)
	{
		return line;
	}
	
	std::stringstream strs(line);
	std::string retLine;
	std::string tmp;
	
	while(getline(strs, tmp, ' '))
	{
		retLine += tmp;
		std::cout << "Tokenizer: " << tmp << " added to: " << retLine << std::endl;
		retLine += " ";
	}
	
	return retLine;
}
