#ifndef RECOGNITION_RESULT_H
#define RECOGNITION_RESULT_H

#include <vector>
#include <chrono>

#include "CustomPostProc.h" 

/**
 *
 * Atomic part of "raw" detailed recognizer output. Can contain subword markers. 
 */
class RecognizedToken
{
public:
	std::string               m_text;
	std::chrono::milliseconds m_duration;
	std::chrono::milliseconds m_relStart;
	std::chrono::milliseconds m_relEnd;
	float                     m_confidence;
	
	RecognizedToken(char* text, unsigned int durationMs, unsigned int startTimeMs, unsigned int endTimeMs, float confidence) 
	{
		m_text             = text;
		m_duration         = std::chrono::milliseconds(durationMs);
		m_relStart         = std::chrono::milliseconds(startTimeMs);
		m_relEnd           = std::chrono::milliseconds(endTimeMs);
		m_confidence       = confidence;
	}
};

/**
 *
 * Words assembled from tokens. Additional spell checking generates additional hint for confidence.
 */
class RecognizedWord
{
public:
	std::string               m_text;
	std::string               m_replacer;
	std::chrono::milliseconds m_duration;
	std::chrono::milliseconds m_relStart;
	std::chrono::milliseconds m_relEnd;
	float                     m_meanConfidence;
	bool                      m_correctSpelling;
	
	RecognizedWord(char* text, char* replacer, unsigned int durationMs, unsigned int startTimeMs, unsigned int endTimeMs, float confidence, bool correctSpelling) 
	{
		m_text             = text;
		m_replacer         = replacer;
		m_duration         = std::chrono::milliseconds(durationMs);
		m_relStart         = std::chrono::milliseconds(startTimeMs);
		m_relEnd           = std::chrono::milliseconds(endTimeMs);
		m_meanConfidence   = confidence;
		m_correctSpelling  = correctSpelling;
	}
};

/**
 *
 * Words that belong to the same utterance.
 */
class RecognizedUtterance
{
public:
	
	uint64_t    m_frameCounterStart;
	uint64_t    m_frameCounterEnd;
    int64_t     m_uStartTime;
    int64_t     m_uStartTimeMs;
    int64_t     m_uStopTime;
    int64_t     m_uStopTimeMs;
    int         m_frameResolutionMs;
    
    RecognizedUtterance(uint64_t frameCounterStart, uint64_t frameCounterEnd, int64_t uStartTime, int64_t uStartTimeMs,
    	int64_t uStopTime, int64_t uStopTimeMs, int frameResolutionMs, CustomPostProc* cpp)
    {
    	m_frameCounterStart = frameCounterStart;
    	m_frameCounterEnd   = frameCounterEnd;
    	m_uStartTime        = uStartTime;
    	m_uStartTimeMs      = uStartTimeMs;
    	m_uStopTime         = uStopTime;
    	m_uStopTimeMs       = uStopTimeMs;
    	m_frameResolutionMs = frameResolutionMs;
    	
    	m_totalUtterance = "";
    	m_meanConfidence = 0.0f;
    	m_saneSize       = 0;
    	
    	m_sanitized = false;
    	
    	m_cpp = cpp;
    	
    	words.clear();
    }
    
    void addWord(std::unique_ptr<RecognizedWord> word)
    {
    	words.push_back(std::move(word));
    }
    
    std::string getTotalUtterance()
    {
    	if (!m_sanitized)
    	{
    		sanitize();
    	}
    	
    	return m_totalUtterance;
    }
    
    uint32_t getNumberWords()
    {
    	if (!m_sanitized)
    	{
    		sanitize();
    	}
    	
    	return m_saneSize;	
    }
    
    std::unique_ptr<RecognizedWord> popWord(uint32_t index)
    {
    	if (index >= m_saneSize)
    	{
    		return nullptr;	
    	}
    	
    	return std::move(words[index]);
    }
    
    ~RecognizedUtterance()
    {
    	words.clear();	
    }
    
private:
	
    std::vector<std::unique_ptr<RecognizedWord>> words;
	std::string m_totalUtterance;
    float       m_meanConfidence;
    bool        m_sanitized;
	uint32_t    m_saneSize;
	
	CustomPostProc* m_cpp;
    
    void sanitize()
    {
    	// 1) construct whole utterance text (recognized content)
		for (unsigned int i = 0; i < words.size(); i++)
		{
			if (i == 0)
			{
				m_totalUtterance = words[i]->m_text;
			}
			else
			{
				m_totalUtterance = m_totalUtterance + " " + words[i]->m_text;	
			}
		}
    	
		// 2) run line limiter to find out max. allowed length
		
		// compute utterance length based on frame counter, not on timestamps
		// timestamps are only valid for online mode, not offline transcripts!!!
		
		uint64_t frameCounterDiff = m_frameCounterEnd - m_frameCounterStart;
		float frameLenMs = m_frameResolutionMs * frameCounterDiff;
		int lengthInSeconds = (int) (frameLenMs + 1000);
		
		int maxLineLen = m_cpp->limitLine(m_totalUtterance, lengthInSeconds);
		if (maxLineLen == -1)
		{
			maxLineLen = m_totalUtterance.length();
		}
		
		// 3) recreate total utterance based on limited string (until longer)
		// always need to do this to use replaced words
		m_totalUtterance = "";
		unsigned int i = 0;
		while ((m_totalUtterance.length() <= ((unsigned int) maxLineLen)) && (i < words.size()))
		{
			if (i == 0)
			{
				m_totalUtterance = words[i]->m_replacer;
			}
			else
			{
				m_totalUtterance = m_totalUtterance + " " + words[i]->m_replacer;	
			}
			
			m_meanConfidence += words[i]->m_meanConfidence;
			
			i++;
		}
		m_meanConfidence = m_meanConfidence / ((float) i);
		
		// 4) set new maximum of (usable) words
		m_saneSize = i;
    }
};

#endif // RECOGNITION_RESULT_H
