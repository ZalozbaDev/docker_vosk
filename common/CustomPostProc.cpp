
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
			tmpVal = replace_all(tmpVal, replacees[index], replacers[index], maxsuffixes[index]);	
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
			
			std::string left, middle, right, tmp;
			
			left  = oneLine.substr(0, delimPos);
			tmp = oneLine.substr(delimPos + 1, oneLine.length() - delimPos);
			
			delimPos = tmp.find('\t');
			// invalid file means do not use this functionality
			if (std::string::npos == delimPos) {
				return false;
			}
			
			middle  = tmp.substr(0, delimPos);
			right = tmp.substr(delimPos + 1, tmp.length() - delimPos);
			
			
			std::cout << "Replacement line: '" << left << "' replaced by '" << middle << "', maxsuffixes=" << right << "." << std::endl;
			
			replacees.push_back(left);
			replacers.push_back(middle);
			maxsuffixes.push_back(std::stoi(right));
		}
	}
	
	return true;
}

//////////////////////////////////////////////
std::string CustomPostProc::replace_all(std::string line, std::string replacee, std::string replacer, std::size_t maxSuffix)
{
    std::string retVal;
    std::size_t currPos = 0;
    std::size_t prevPos;
    std::size_t nextSpace;

    // might increase performance
    retVal.reserve(line.size());

    while (true) 
    {
        prevPos = currPos;

        currPos = line.find(replacee, currPos);
        if (currPos == std::string::npos)
            break;

        if (currPos != 0)
        {
        	// check if a space is before the word
        	// we should never replace if not at beginning of word
        	if (line[currPos - 1] != ' ')
        	{
        		std::cout << "Skip match that is not at beginning of word!" << std::endl;
        		
        		retVal.append(line, prevPos, currPos - prevPos);
        		retVal += replacee;
        		currPos += replacee.size();
        		
        		continue;
        	}
        }
        // currPos == 0 means word starts at line start, so this is OK
        
        // find next space after match to decide if to be replaced
        nextSpace = line.find(" ", (currPos + replacee.size()));
        
        if (std::string::npos == nextSpace)
        {
        	// no more spaces, compute from string length
        	nextSpace = line.size() - currPos - replacee.size();
        	
        	std::cout << "At string end, size=" << line.size() << ", currPos=" << currPos << ", replacee=" << replacee.size() << "." << std::endl;
        }
        else
        {
        	std::cout << "currPos=" << currPos << ", found nextSpace=" << nextSpace << ", subtracting." << std::endl;
        	
        	nextSpace -= (currPos + replacee.size());	
        }
        
        std::cout << "Match of '" << replacee << "' occurred, next space found at distance " 
        	      << nextSpace << ", max is " << maxSuffix << "." << std::endl;
        
        retVal.append(line, prevPos, currPos - prevPos);

        if (nextSpace <= maxSuffix)
        {
        	retVal += replacer;
        }
        else
        {
        	std::cout << "Not replacing match, suffix too long!" << std::endl;
        	
        	retVal += replacee;
        }
        
        currPos += replacee.size();
    }

    retVal.append(line, prevPos, line.size() - prevPos);
    
    return retVal;
}
