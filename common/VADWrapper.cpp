
#include <VADWrapper.h>

#include <iostream>

#include <cassert>
#include <cstring>

#include <chrono>

//////////////////////////////////////////////
VADWrapper::VADWrapper(int aggressiveness, size_t frequencyHz, unsigned int audioPreBufferFrames,
	unsigned int audioPostBufferFrames, unsigned int vadHystheresisFramesOn, unsigned int vadHystheresisFramesOff) :
	m_audioPreBufferFrames(audioPreBufferFrames), m_audioPostBufferFrames(audioPostBufferFrames), 
	m_vadHystheresisFramesOn(vadHystheresisFramesOn), m_vadHystheresisFramesOff(vadHystheresisFramesOff)
{
	int status;
	
	rtcVadInst = WebRtcVad_Create();
	
	status = WebRtcVad_Init(rtcVadInst);
	if (status != 0)
	{
		std::cout << "WebRtcVad_Init not successful!" << std::endl;
	}
	
	status = WebRtcVad_set_mode(rtcVadInst, aggressiveness);
	if (status != 0)
	{
		std::cout << "WebRtcVad_set_mode not successful!" << std::endl;
	}
	
	status = WebRtcVad_ValidRateAndFrameLength(frequencyHz, nrVADSamples);
	if (status != 0)
	{
		std::cout << "Invalid combination of sample rate and number of samples!" << std::endl;	
	}
	
	leftOverSampleSize = 0;
	
	state = VADWrapperState::IDLE;
}

//////////////////////////////////////////////
VADWrapper::~VADWrapper(void)
{
	chunks.clear();
	
	WebRtcVad_Free(rtcVadInst);
}

//////////////////////////////////////////////
//
// process all data provided
// all data is VAD analyzed and stored in the "chunks" vector
//
//////////////////////////////////////////////
int VADWrapper::process(int samplingFrequency, const int16_t* audio_frame, size_t frame_length, std::uint64_t frameCtr, std::chrono::time_point<std::chrono::system_clock> frameTime)
{
	int result, retVal;
	size_t frame_ptr;
	
	// leftover samples handling done at upper layer, can assume one full frame per call
	assert(frame_length == nrVADSamples);
	
	retVal = 0;

	std::unique_ptr<VADFrame<nrVADSamples>> chunk = std::make_unique<VADFrame<nrVADSamples>>();

	chunk->currFrameCtr  = frameCtr;
	chunk->currFrameTime = frameTime; 
		
	memcpy(chunk->samples, audio_frame, sizeof(chunk->samples));

	// actual VAD processing
	result = WebRtcVad_Process(rtcVadInst, samplingFrequency, chunk->samples, nrVADSamples);
		
	if (result == -1)
	{
		std::cout << "Error processing VAD data!" << std::endl;
		retVal = -1;
	}
		
	// 1 == active, 0 == not active, -1 == error
	chunk->state = (result == 1) ? VADState::ACTIVE : VADState::OFF;

#ifdef VAD_FRAME_CONVERT_FLOAT	
	// we need to convert every frame to float for whisper
	// because we dont know which range is used for recognition
	for (unsigned int tmp = 0; tmp < nrVADSamples; tmp++)
	{
		chunk->fSamples[tmp] = (float) (((double) chunk->samples[tmp]) / 32768.0); 
	}
#endif		
		
	chunks.push_back(std::move(chunk));
	
	return retVal;
}

//////////////////////////////////////////////
//
// process the chunks vector
// shrink until first utterance starts or minimum size of vector reached
//
// returns true if there is no data to fetch for recognition
//
//////////////////////////////////////////////
bool VADWrapper::analyze(bool hintShortAudio)
{
	switch (state)
	{
		// utterance has not started
		case VADWrapperState::IDLE:
			bool success;
			success = findUtteranceStart();
			if (success == true)
			{
				findUtteranceStop(hintShortAudio);
			}
			break;
		// utterance start detected, checking for stop
		case VADWrapperState::ACTIVE:
			findUtteranceStop(hintShortAudio);
			break;
		// utterance start and stop detected, duplicate data for the postbuf period
		case VADWrapperState::POSTBUF:
			// nothing to analyze, wait for postbuf data drained before going idle
			break;
	}
	
	// either we are still idle or all audio has been consumed
	return ((state == VADWrapperState::IDLE) || (chunks.size() == 0)) ? true : false;
}

//////////////////////////////////////////////
unsigned int VADWrapper::getAvailableChunks(void)
{
	switch (state)
	{
		case VADWrapperState::IDLE:
			// don't feed irrelevant silence to recognizer 
			return 0;
		case VADWrapperState::ACTIVE:
			// all chunks can be read
			return chunks.size();
		case VADWrapperState::POSTBUF:
			// read chunks until the end of utterance was analyzed
			return (utteranceCurr + 1);
	}
	
	assert(false);
	
	return 0;
}

//////////////////////////////////////////////
bool VADWrapper::findUtteranceStart(void)
{
	assert(state == VADWrapperState::IDLE);
	
	unsigned int numberActiveFrames = 0;
	unsigned int numberToggles      = 0;
	VADState lastState              = VADState::OFF;
	
	// estimated utterance start chunk
	unsigned int chunkUttStart;	
	
	unsigned int chunksChopOffIdx = 0;
	
	// last chunk analyzed before utterance start detected
	unsigned int chunksAnalyzedStart = 0;
	
	///////////////////////////////////////////////////
	// 1. search through all stored chunks for a possible utterance start (with toggle)
	///////////////////////////////////////////////////
	
	for (unsigned int i = 0; i < chunks.size(); i++)
	{
		// remember if VAD toggles
		if (lastState != chunks[i]->state)
		{
			numberToggles++;
		}
		
		// count consecutive active frames
		if (chunks[i]->state == VADState::ACTIVE)
		{
			numberActiveFrames++;
		}
		else
		{
			if (numberActiveFrames > 0) 
			{
				numberActiveFrames--;
			}
			else
			{
				// reset toggles if too many inactive frames
				numberToggles = 0;
			}
		}
		
		// check criteria for "start found": enough active frames
		if (numberActiveFrames >= m_vadHystheresisFramesOn)
		{
			assert(i >= m_vadHystheresisFramesOn);
			
			state = VADWrapperState::ACTIVE;
			chunkUttStart = i - m_vadHystheresisFramesOn;
			chunksAnalyzedStart = i;
			break;
		}
		// or: heavy toggling
		if ((numberActiveFrames > 0) && (numberToggles > vadMaxNrToggles))
		{
			assert(i >= vadMaxNrToggles);
			
			state = VADWrapperState::ACTIVE;
			chunkUttStart = i - vadMaxNrToggles;
			chunksAnalyzedStart = i;
			break;
		}
		
		lastState = chunks[i]->state;
	}
	
	///////////////////////////////////////////////////
	// 2. remember properties of start chunk
	///////////////////////////////////////////////////
	
	if (state == VADWrapperState::ACTIVE)
	{
		frameCtrStart = chunks[chunkUttStart]->currFrameCtr;
		
		std::chrono::time_point<std::chrono::system_clock> timeStampStart = chunks[chunkUttStart]->currFrameTime;
	
		uStartTime   = std::chrono::duration_cast<std::chrono::seconds>(timeStampStart.time_since_epoch()).count();
		uStartTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeStampStart.time_since_epoch()).count() - (uStartTime * 1000);
	}
	
	
	///////////////////////////////////////////////////
	// 3. compute possible frames to be discarded
	///////////////////////////////////////////////////
	
	if (state == VADWrapperState::IDLE)
	{
		// delete all old frames, keep prebuf frames only	
		if (chunks.size() > m_audioPreBufferFrames)
		{
			chunksChopOffIdx = chunks.size() - m_audioPreBufferFrames;
		}
	}
	else
	{
		// keep prebuf frames before utterance start, remove older
		if (chunkUttStart > m_audioPreBufferFrames)
		{
			chunksChopOffIdx = chunkUttStart - m_audioPreBufferFrames;
		}
	}
	
	///////////////////////////////////////////////////
	// 4. chop off old chunks
	///////////////////////////////////////////////////
	
	if (chunksChopOffIdx > 0)
	{
		chunks.erase(chunks.begin(), chunks.begin() + (chunksChopOffIdx - 1));	
	}
	
	///////////////////////////////////////////////////
	// 5. return if start found
	///////////////////////////////////////////////////
	
	if (state == VADWrapperState::ACTIVE)
	{
		m_analyzeStopOffset = chunksAnalyzedStart - chunksChopOffIdx;
		return true;
	}
	else
	{
		assert(chunks.size() == m_audioPreBufferFrames);
		return false;	
	}
}

//////////////////////////////////////////////
void VADWrapper::findUtteranceStop(bool hintShortAudio)
{
	assert(state == VADWrapperState::ACTIVE);
	
	unsigned int searchStart = m_analyzeStopOffset;
	
	unsigned int chunkUttStopCtr = 0; 
	
	unsigned int chunkUttEnd;	
	
	///////////////////////////////////////////////////
	// 1. find possible end of utterance
	///////////////////////////////////////////////////
	for (unsigned int i = searchStart; i < chunks.size(); i++)
	{
		if (chunks[i]->state == VADState::OFF)
		{
			chunkUttStopCtr++;
		}
		else
		{
			chunkUttStopCtr = 0;	
		}
		
		if (chunkUttStopCtr >= m_vadHystheresisFramesOff)
		{
			VADWrapperState::POSTBUF;
			chunkUttEnd = i;
			break;
		}
	}
		
	///////////////////////////////////////////////////
	// 2. remember search props / assign utterance end props
	///////////////////////////////////////////////////
	if (state == VADWrapperState::ACTIVE)
	{
		m_analyzeStopOffset = chunks.size();	
	}
	else
	{
		frameCtrStop = chunks[chunkUttEnd]->currFrameCtr;
		std::chrono::time_point<std::chrono::system_clock> timeStampStop = chunks[chunkUttEnd]->currFrameTime;
		
		uStopTime   = std::chrono::duration_cast<std::chrono::seconds>(timeStampStop.time_since_epoch()).count();
		uStopTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeStampStop.time_since_epoch()).count() - (uStopTime * 1000);
	}
	
	///////////////////////////////////////////////////
	// 3. initialize postbuf logic
	///////////////////////////////////////////////////
	if (state == VADWrapperState::POSTBUF)
	{
		m_unbufferedStopChunks = frameCtrStop;
		m_bufferedStopChunks   = m_audioPostBufferFrames;
	}
}

//////////////////////////////////////////////
std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> VADWrapper::getNextChunk(void)
{
	std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk;
	
	assert(state != VADWrapperState::IDLE);
	assert(chunks.size() > 0);

	// return whatever is in queue in active state
	if (state == VADWrapperState::ACTIVE)
	{
		// this moves the element to the local var but keeps an invalid (maybe null) entry in the deque
		chunk = std::move(chunks.front());
		
		// the invalid entry needs to be deleted
		chunks.pop_front();
	}
	
	if (state == VADWrapperState::POSTBUF)
	{
		// supply audio until end-of-utterance and remove from queue
		if (m_unbufferedStopChunks > 0)
		{
			// this moves the element to the local var but keeps an invalid (maybe null) entry in the deque
			chunk = std::move(chunks.front());
			
			// the invalid entry needs to be deleted
			chunks.pop_front();
			
			m_unbufferedStopChunks--;
		}
		else
		{
			// copy the buffered chunks only, they shall be analyzed for a next possible start
			assert(m_bufferedStopChunks > 0);
	
			memcpy(chunk.samples, chunks[0].samples, sizeof(chunk.samples));
#ifdef VAD_FRAME_CONVERT_FLOAT	
			memcpy(chunk.fsamples, chunks[0].fsamples, sizeof(chunk.fsamples));
#endif
			chunk.state         = chunks[0].state;
			chunk.currFrameCtr  = chunks[0].currFrameCtr;
			chunk.currFrameTime = chunks[0].currFrameTime
			
			m_bufferedStopChunks--;
			if (m_bufferedStopChunks == 0)
			{
				state = VADWrapperState::IDLE;	
			}
		}
	}
	
	// return the current (copied / moved) chunk
	return (chunk);
}

