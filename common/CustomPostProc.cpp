
#include <iostream>
#include <fstream>

#include <CustomPostProc.h>

//////////////////////////////////////////////
CustomPostProc::CustomPostProc(bool active, std::string replacementFile)
{
	passThrough = !active;
	
	if (replacementFile.length() < 1)
	{
		listReplace = false;
	}
	else
	{
		listReplace = readReplacementFile(replacementFile);
		
		if (listReplace == false)
		{
			std::cout << "File " << replacementFile << " is invalid - ignored!" << std::endl;	
		}
	}
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
	
	// iterate through list and replace all occurences with their counterpart
	if (listReplace == true)
	{
		std::string tmpVal = retVal;
		
		for (size_t index = 0; index < replacees.size(); index++)
		{
			tmpVal = replace_all(tmpVal, replacees[index], replacers[index]);	
		}
		
		retVal = tmpVal;
	}
	
	std::cout << "Orig: '" << line << "' changed to '" << retVal << "'" << std::endl;
	
	return retVal;
}

//////////////////////////////////////////////
bool CustomPostProc::readReplacementFile(std::string filename)
{
	replacees.clear();
	replacers.clear();
	
	std::ifstream myList(filename);
	
	while (!myList.eof())
	{
		std::string oneLine;
		
		getline(myList, oneLine);
		
		if (oneLine.length() > 0)
		{
			size_t delimPos = oneLine.find('\t');
			
			// invalid file means do not use this functionality
			if (std::string::npos == delimPos) {
				return false;
			}
			
			std::string left, right;
			
			left  = oneLine.substr(0, delimPos);
			right = oneLine.substr(delimPos + 1, oneLine.length() - delimPos);
			
			std::cout << "Replacement line: '" << left << "' replaced by '" << right << "'." << std::endl;
			
			replacees.push_back(left);
			replacers.push_back(right);
		}
	}
	
	return true;
}

//////////////////////////////////////////////
std::string CustomPostProc::replace_all(std::string line, std::string replacee, std::string replacer)
{
    std::string retVal;
    std::size_t currPos = 0;
    std::size_t prevPos;

    // might increase performance
    retVal.reserve(line.size());

    while (true) 
    {
        prevPos = currPos;
        
        currPos = line.find(replacee, currPos);
        if (currPos == std::string::npos)
            break;
        
        retVal.append(line, prevPos, currPos - prevPos);
        retVal += replacer;
        
        currPos += replacee.size();
    }

    retVal.append(line, prevPos, line.size() - prevPos);
    
    return retVal;
}
