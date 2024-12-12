
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/locid.h>

#include <WordClassPostProc.h>

#include "exprtk.hpp"

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
	bool searchFinished = false;
	
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(line, "CURRENCY");
		
		if (substr->beginTag != std::string::npos)
		{
		
			std::cout << "Try to match CURRENCY: " << substr->beginExpr << "-" << substr->endExpr << "." << std::endl;
			
			std::string result = evalMathExpr(line.substr(substr->beginExpr, substr->endExpr - substr->beginExpr));
			
			std::cout << "Formatted currency is " << result << "." << std::endl;
			
			retVal = replaceWordClass(retVal, std::move(substr), result + "€");
		}
		
		searchFinished = true;
	}
	
	
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

//////////////////////////////////////////////
std::unique_ptr<wc_substr> WordClassPostProc::findTags(std::string line, std::string tagName)
{
	std::string beginTag = "<" + tagName + ">";
	std::string endTag   = "</" + tagName + ">";
	std::unique_ptr<wc_substr> substr = std::make_unique<wc_substr>();
	
	substr->beginTag  = line.find(beginTag);
	substr->beginExpr = line.find(beginTag) + beginTag.length();
	substr->endExpr   = line.find(endTag);
	substr->endTag    = line.find(endTag) + endTag.length();
	
	return substr;
}

//////////////////////////////////////////////
std::string WordClassPostProc::evalMathExpr(std::string input)
{
	exprtk::parser<float>     parser;
	exprtk::expression<float> expression;
	
	if (!parser.compile(input, expression))
	{
		std::cout << "Error compiling expression:" << input << "." << std::endl;
		return input;
	}
	
	float result = expression.value();
	
	std::cout << "Expression result is " << result << "." << std::endl;
	
	std::stringstream strs;
	strs << std::fixed << std::setprecision(2) << result;
	
	return strs.str();
}

//////////////////////////////////////////////
std::string WordClassPostProc::replaceWordClass(std::string line, std::unique_ptr<wc_substr> substr, std::string replacer)
{
	std::string retVal;
	
	retVal = line.substr(0, substr->beginTag);
	retVal += replacer;
	retVal += line.substr(substr->endTag);
	
	return retVal;
}
