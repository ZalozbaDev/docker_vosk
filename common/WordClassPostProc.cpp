
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

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
	int endlessLoopAbort = 0;
	
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
			std::string result = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 2);
			retVal = replaceWordClass(retVal, std::move(substr), result + "€");
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
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
			std::string result = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 1);
			retVal = replaceWordClass(retVal, std::move(substr), result + "%");
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
	}
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// DATE
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "DATE");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
			std::string result1;
			int date1offset;
			std::string result2 = evalErrorRes;
			int date2offset = notComputedValueOffset;
			size_t delimiter_pos;
			
			// std::cout << "Try to match DATE: " << substr->beginExpr << "-" << substr->endExpr << "." << std::endl;

			delimiter_pos = retVal.find(dateTimeDelimiter, substr->beginExpr);
			
			if ((delimiter_pos == std::string::npos) || (delimiter_pos >= substr->endExpr))
			{
				result1 = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 0);
				if (result1.compare(evalErrorRes) != 0)
				{
					date1offset = std::stoi(result1);	
				}
				else
				{
					date1offset = invalidValueOffset;	
				}
			}
			else
			{
				result1 = evalMathExpr(retVal.substr(substr->beginExpr, delimiter_pos - substr->beginExpr), 0);
				result2 = evalMathExpr(retVal.substr(delimiter_pos + dateTimeDelimiter.length(), substr->endExpr - delimiter_pos - dateTimeDelimiter.length()), 0);
				
				if (result1.compare(evalErrorRes) != 0)
				{
					date1offset = std::stoi(result1);	
				}
				else
				{
					date1offset = invalidValueOffset;	
				}
				
				if (result2.compare(evalErrorRes) != 0)
				{
					date2offset = std::stoi(result2);	
				}
				else
				{
					date2offset = invalidValueOffset;	
				}
			}

			// std::cout << "Date parser result1=" << result1 << "(" << date1offset << "), result2=" << result2 << "(" << date2offset << ")." << std::endl;
			
			std::string formatted;
			
			if (date1offset != invalidValueOffset)
			{
				formatted = evalDateOffset(date1offset);
			}
			else
			{
				formatted = evalErrorRes;	
			}
			
			if (date2offset != notComputedValueOffset)
			{
				if (date2offset != invalidValueOffset)
				{
					formatted += "-" + evalDateOffset(date2offset);
				}
				else
				{
					formatted += "-" + evalErrorRes;
				}
			}
			
			// std::cout << "Formatted date is " << formatted << std::endl;
			
			retVal = replaceWordClass(retVal, std::move(substr), formatted);
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
	}
	
	
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// TIME
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "TIME");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
			std::string result1;
			int time1offset;
			std::string result2 = evalErrorRes;
			int time2offset = notComputedValueOffset;
			size_t delimiter_pos;
			
			std::cout << "Try to match TIME: " << substr->beginExpr << "-" << substr->endExpr << "." << std::endl;

			delimiter_pos = retVal.find(dateTimeDelimiter, substr->beginExpr);
			
			if ((delimiter_pos == std::string::npos) || (delimiter_pos >= substr->endExpr))
			{
				result1 = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 0);
				if (result1.compare(evalErrorRes) != 0)
				{
					time1offset = std::stoi(result1);	
				}
				else
				{
					time1offset = invalidValueOffset;	
				}
			}
			else
			{
				result1 = evalMathExpr(retVal.substr(substr->beginExpr, delimiter_pos - substr->beginExpr), 0);
				result2 = evalMathExpr(retVal.substr(delimiter_pos + dateTimeDelimiter.length(), substr->endExpr - delimiter_pos - dateTimeDelimiter.length()), 0);
				
				if (result1.compare(evalErrorRes) != 0)
				{
					time1offset = std::stoi(result1);	
				}
				else
				{
					time1offset = invalidValueOffset;	
				}
				
				if (result2.compare(evalErrorRes) != 0)
				{
					time2offset = std::stoi(result2);	
				}
				else
				{
					time2offset = invalidValueOffset;	
				}
			}

			std::cout << "Time parser result1=" << result1 << "(" << time1offset << "), result2=" << result2 << "(" << time2offset << ")." << std::endl;
			
			std::string formatted;
			
			if (time1offset != invalidValueOffset)
			{
				formatted = evalTimeOffset(time1offset);
			}
			else
			{
				formatted = evalErrorRes;	
			}
			
			if (time2offset != notComputedValueOffset)
			{
				if (time2offset != invalidValueOffset)
				{
					formatted += "-" + evalTimeOffset(time2offset);
				}
				else
				{
					formatted += "-" + evalErrorRes;
				}
			}
			
			std::cout << "Formatted time is " << formatted << std::endl;
			
			retVal = replaceWordClass(retVal, std::move(substr), formatted);
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
	}
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// ORDINAL
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "ORDINAL");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
			std::string result = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 0);
			retVal = replaceWordClass(retVal, std::move(substr), result + ".");
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
	}
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// CARDINAL
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "CARDINAL");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
			std::string result = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 0);
			retVal = replaceWordClass(retVal, std::move(substr), result);
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
	}
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// NAME
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "NAME");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
			std::string result = retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr);
			retVal = replaceWordClass(retVal, std::move(substr), result);
			// std::cout << "Orig: '" << line << "' changed to '" << retVal.substr(0, 100) << "'" << std::endl;
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
	}
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// SNAME
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "SNAME");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
			std::string result = retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr);
			retVal = replaceWordClass(retVal, std::move(substr), result);
			// std::cout << "Orig: '" << line << "' changed to '" << retVal.substr(0, 100) << "'" << std::endl;
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
	}
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// GPE
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "GPE");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
			std::string result = retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr);
			retVal = replaceWordClass(retVal, std::move(substr), result);
			// std::cout << "Orig: '" << line << "' changed to '" << retVal.substr(0, 100) << "'" << std::endl;
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
	}
	
	searchFinished = false;
	
	//////////////////////////////
	//
	// WEEKDAY
	//
	//////////////////////////////
	while(searchFinished == false)
	{
		std::unique_ptr<wc_substr> substr;
		
		substr = findTags(retVal, "WEEKDAY");
		searchFinished = !substr->valid;
		
		if (searchFinished == false)
		{
			std::string result = evalMathExpr(retVal.substr(substr->beginExpr, substr->endExpr - substr->beginExpr), 0);
			
			int weekdayIndex = std::stoi(result);
			
			if ((weekdayIndex > 0) && (weekdayIndex < 8))
			{
				result = weekdays[weekdayIndex - 1];	
			}
			else
			{
				result = evalErrorRes;
			}
						
			retVal = replaceWordClass(retVal, std::move(substr), result);
		}
		
		if ((endlessLoopAbort++) > maxLoopCounter) return retVal;
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

	substr->valid = ((substr->beginTag == std::string::npos) || (substr->endExpr == std::string::npos)) ? false : true;
	
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
		return evalErrorRes;
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

//////////////////////////////////////////////
std::string WordClassPostProc::evalDateOffset(int dateOffset)
{
	tm start_date = {};
	
	std::stringstream strs;
	
	if (dateOffset <= 0)
	{
		start_date.tm_year = 2024-1900; // year since 1900
		start_date.tm_mon  = 11;        // December
		start_date.tm_mday = 31;
		
		time_t start_time = mktime(&start_date);
		if (start_time == -1)
		{
			return evalErrorRes;
		}
		
		time_t offset_time = start_time + (dateOffset * 24 * 3600); // offset in seconds
		
		tm* target_date = localtime(&offset_time);
		
		strs << std::setw(2) << std::setfill('0') << target_date->tm_mday << "." 
		     << std::setw(2) << std::setfill('0') << (target_date->tm_mon + 1) << ".";
	}
	else
	{
		start_date.tm_year = 2024-1900; // year since 1900
		start_date.tm_mon  = 0;         // January
		start_date.tm_mday = 1;
		
		time_t start_time = mktime(&start_date);
		if (start_time == -1)
		{
			return evalErrorRes;
		}
		
		time_t offset_time = start_time + ((dateOffset - 1) * 24 * 3600); // offset in seconds
		
		tm* target_date = localtime(&offset_time);
		
		strs << std::setw(2) << std::setfill('0') << target_date->tm_mday << "." 
		     << std::setw(2) << std::setfill('0') << (target_date->tm_mon + 1) << ".";
	}
	
	return strs.str();
}

//////////////////////////////////////////////
std::string WordClassPostProc::evalTimeOffset(int timeOffset)
{
	std::stringstream strs;
	
	int hours = timeOffset / 60;
	int minutes = timeOffset % 60;
	
	strs << std::setw(2) << std::setfill('0') << hours << ":" 
	     << std::setw(2) << std::setfill('0') << minutes;
	
	return strs.str();
}
