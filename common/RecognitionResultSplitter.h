#ifndef RECOGNITION_RESULT_SPLITTER_H
#define RECOGNITION_RESULT_SPLITTER_H

#include <vector>
#include <chrono>

#include "RecognitionResult.h"

class RecognitionResultSplitter
{
public:
	RecognitionResultSplitter(void);
	pushResult(std::unique_ptr<TimedRecognitionResult> result);
	std::unique_ptr<TimedRecognitionResult> pullResultPieces(void);
private:
	std::vector<std::unique_ptr<TimedRecognitionResult>> resultPieces;
};

#endif // RECOGNITION_RESULT_SPLITTER_H
