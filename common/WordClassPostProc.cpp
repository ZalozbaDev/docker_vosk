
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
	
	//////////////////////////////
	//
	// CURRENCY
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "CURRENCY");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
		
			// std::cout << "Try to match CURRENCY: " << substr->beginExpr << "-" << substr->endExpr << "." << std::endl;
			
			std::string result = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 2);
			
			// std::cout << "Formatted currency is " << result << "." << std::endl;
			
			retVal = replaceWordClass(retVal, std::move(substr), result + "€");
		}
	}
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// PERCENT
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "PERCENT");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
		
			std::cout << "Try to match PERCENT: " << substr->beginExpr << "-" << substr->endExpr << "." << std::endl;
			
			std::string result = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 1);
			
			std::cout << "Formatted percentage is " << result << "." << std::endl;
			
			retVal = replaceWordClass(retVal, std::move(substr), result + "%");
		}
	}
	
	
	
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

	substr->valid = (substr->beginTag == std::string::npos) ? false : true;
	
	return substr;
}

//////////////////////////////////////////////
std::string WordClassPostProc::evalMathExpr(std::string input, int precision)
{
	exprtk::parser<float>     parser;
	exprtk::expression<float> expression;
	
	if (!parser.compile(input, expression))
	{
		std::cout << "Error compiling expression:" << input << "." << std::endl;
		return "???";
	}
	
	float result = expression.value();
	
	std::cout << "Expression result is " << result << "." << std::endl;
	
	std::stringstream strs;
	strs << std::fixed << std::setprecision(precision) << result;
	
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
