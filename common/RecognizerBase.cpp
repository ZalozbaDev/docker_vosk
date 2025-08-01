
#include "RecognizerBase.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

//////////////////////////////////////////////////////////////////////////////
RecognizerBase::~RecognizerBase()
{
	
}

//////////////////////////////////////////////////////////////////////////////
std::string RecognizerBase::getLocalTimeStamp()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto str = oss.str();

    return str;
}
