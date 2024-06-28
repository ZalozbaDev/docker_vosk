#ifndef RECOGNITION_RESULT_SPLITTER_H
#define RECOGNITION_RESULT_SPLITTER_H

#include <vector>
#include <chrono>

#include "RecognitionResult.h"

class RecognitionResultSplitter
{
public:
	RecognitionResultSplitter(void);
	pushResult(RecognitionResult result);
	RecognitionResult pullResultPieces(void);
private:
	std::vector<RecognitionResult> resultPieces;
};

#endif // RECOGNITION_RESULT_SPLITTER_H
