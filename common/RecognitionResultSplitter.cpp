
#include "RecognitionResultSplitter.h"

//////////////////////////////////////////////
RecognitionResultSplitter::RecognitionResultSplitter(void)
{
	resultPieces.clear();
}

//////////////////////////////////////////////
RecognitionResultSplitter::~RecognitionResultSplitter(void)
{
	resultPieces.clear();
}

//////////////////////////////////////////////
void RecognitionResultSplitter::pushResult(std::unique_ptr<TimedRecognitionResult> result)
{
	if ((result.end - result.start) < 5)
	{
		resultPieces.push_back(std::move(result));	
	}
}

//////////////////////////////////////////////
std::unique_ptr<TimedRecognitionResult> RecognitionResultSplitter::pullResultPieces(void)
{
	return nullptr;
}
