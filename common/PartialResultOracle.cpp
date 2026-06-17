#include "PartialResultOracle.h"

#include <iostream>

//////////////////////////////////////////////
PartialResultOracle::PartialResultOracle(std::chrono::milliseconds resultPeriod)
{
	m_resultPeriod = resultPeriod;
	m_length = std::chrono::milliseconds(0);
	m_currChunks = 0;
	partialPending = false;
}

//////////////////////////////////////////////
void PartialResultOracle::reset()
{
	m_length = std::chrono::milliseconds(0);
	m_currChunks = 0;
	partialPending = false;
}

//////////////////////////////////////////////
void PartialResultOracle::addAudioLength(std::chrono::milliseconds length)
{
	unsigned int newChunks;
	
	m_length += length;
	newChunks = m_length / m_resultPeriod;
	
	if (newChunks > m_currChunks)
	{
		partialPending = true;
		m_currChunks = newChunks;
	}
}

//////////////////////////////////////////////
bool PartialResultOracle::getNextPartialResult()
{
	if (partialPending == true)
	{
		std::cout << "New partial result at " << m_length.count() << "ms." << std::endl;
	}
	
	return partialPending;	
}

//////////////////////////////////////////////
void PartialResultOracle::ackPartialResult()
{
	partialPending = false;	
}

//////////////////////////////////////////////
PartialResultOracle::~PartialResultOracle()
{
	
}

