
#include <iostream>
#include <sstream>
#include <regex>

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
		if (hsp->spell(tmp) == false)
		{
			std::vector<std::string> sugg = hsp->suggest(tmp);
			if (sugg.size() > 0)
			{
				for (unsigned long idx = 0; idx < sugg.size(); idx++)
				{
					std::cout << "Suggestion " << idx << " for word '" << tmp << "': " <<  sugg[idx] << std::endl;
				}
				std::string corr = sugg[0];
				std::cout << "Tokenizer: '" << tmp << "' changed to '" << corr << "' and added to: " << retLine << std::endl;
				retLine += corr;
			}
		}
		else
		{
			std::cout << "Tokenizer: " << tmp << " added to: " << retLine << std::endl;
			retLine += tmp;
		}
		retLine += " ";
	}
	
	retLine = std::regex_replace(retLine, std::regex(" +$"), "");
	
	return retLine;
}
