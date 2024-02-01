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

#endif // RECOGNITION_RESULT_H
