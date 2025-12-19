#ifndef RECOGNITION_RESULT_H
#define RECOGNITION_RESULT_H

#include <vector>
#include <chrono>

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
	std::chrono::milliseconds m_duration;
	std::chrono::milliseconds m_relStart;
	std::chrono::milliseconds m_relEnd;
	float                     m_meanConfidence;
	bool                      m_correctSpelling;
	
	RecognizedWord(char* text, unsigned int durationMs, unsigned int startTimeMs, unsigned int endTimeMs, float confidence, bool correctSpelling) 
	{
		m_text             = text;
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
    
    RecognizedUtterance(uint64_t frameCounterStart, uint64_t frameCounterEnd, int64_t uStartTime, int64_t uStartTimeMs,
    	int64_t uStopTime, int64_t uStopTimeMs)
    {
    	m_frameCounterStart = frameCounterStart;
    	m_frameCounterEnd   = frameCounterEnd;
    	m_uStartTime        = uStartTime;
    	m_uStartTimeMs      = uStartTimeMs;
    	m_uStopTime         = uStopTime;
    	m_uStopTimeMs       = uStopTimeMs;
    	
    	m_totalUtterance = "";
    	m_meanConfidence = 0.0f;
    	m_saneSize       = 0;
    	
    	m_sanizited = false;
    	
    	words.clear();
    }
    
    addWord(std::vector<std::unique_ptr<RecognizedWord>> word)
    {
    	words.push_back(word);
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
    
    void sanitize(void)
    {
    	// construct utterance text
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
    	
		// run line limiter
		
		// recreate total utterance based on limited string (until longer)
    }
};

#endif // RECOGNITION_RESULT_H
