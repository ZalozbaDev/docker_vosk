#ifndef PARTIAL_RESULT_ORACLE_H
#define PARTIAL_RESULT_ORACLE_H

#include <chrono>

class PartialResultOracle
{
public:
	PartialResultOracle(std::chrono::milliseconds resultPeriod);
	void reset();
	void addAudioLength(std::chrono::milliseconds length);
	bool getNextPartialResult();
	void ackPartialResult();
	~PartialResultOracle();
private:
	std::chrono::milliseconds m_resultPeriod;
	std::chrono::milliseconds m_length;
	unsigned int m_currChunks;
	bool partialPending;
};

#endif // PARTIAL_RESULT_ORACLE_H
