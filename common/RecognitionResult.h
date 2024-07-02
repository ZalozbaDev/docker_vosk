#ifndef RECOGNITION_RESULT_H
#define RECOGNITION_RESULT_H

#include <vector>
#include <chrono>

class RecognitionResult
{
public:
	std::string               text;
	std::chrono::milliseconds start;
	std::chrono::milliseconds end;
	float                     m_negLogLikelihood;
	
	RecognitionResult(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood) 
	{
		text               = word;
		start              = std::chrono::milliseconds(startTimeMs);
		end                = std::chrono::milliseconds(endTimeMs);
		m_negLogLikelihood = negLogLikelihood;
	}
};

class TimedRecognitionResult
{
public:
	std::string text;
	time_t      start;
	time_t      end;
	
	TimedRecognitionResult(char* word, time_t startTimeSeconds, time_t endTimeSeconds) 
	{
		text               = word;
		start              = startTimeSeconds;
		end                = endTimeSeconds;
	}
};

#endif // RECOGNITION_RESULT_H
