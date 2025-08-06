
#include <VADWrapperSilero.h>

#include <iostream>

#include <cassert>
#include <cstring>

#include <chrono>

// define the memory for the constant
const unsigned int VADWrapperSilero::nrVADSamples;

//////////////////////////////////////////////
VADWrapperSilero::VADWrapperSilero(size_t frequencyHz, const std::string model_path, unsigned int audioPreBufferFrames,
	unsigned int audioPostBufferFrames, unsigned int vadHystheresisFramesOn, unsigned int vadHystheresisFramesOff) :
	m_audioPreBufferFrames(audioPreBufferFrames), m_audioPostBufferFrames(audioPostBufferFrames), 
	m_vadHystheresisFramesOn(vadHystheresisFramesOn), m_vadHystheresisFramesOff(vadHystheresisFramesOff)
{
	sileroVadInst = new VadIterator(model_path);
	
	state = VADWrapperState::IDLE;
}

//////////////////////////////////////////////
VADWrapperSilero::~VADWrapperSilero(void)
{
	chunks.clear();
	
	delete sileroVadInst;
}

//////////////////////////////////////////////
//
// process all data provided
// all data is VAD analyzed and stored in the "chunks" vector
//
//////////////////////////////////////////////
int VADWrapperSilero::process(int samplingFrequency, const int16_t* audio_frame, size_t frame_length, std::uint64_t frameCtr, std::chrono::time_point<std::chrono::system_clock> frameTime)
{
	int retVal;
	
	// leftover samples handling done at upper layer, can assume one full frame per call
	assert(frame_length == nrVADSamples);
	
	retVal = 0;

	std::unique_ptr<VADFrame> chunk = std::make_unique<VADFrame>(nrVADSamples);

	chunk->currFrameCtr  = frameCtr;
	chunk->currFrameTime = frameTime; 
		
	memcpy(chunk->samples, audio_frame, (chunk->m_numberSamples * sizeof(short)));

	// we need to convert every frame to float already for VAD
	// it does not matter what is used for actual recognition
	for (unsigned int tmp = 0; tmp < nrVADSamples; tmp++)
	{
		chunk->fSamples[tmp] = (float) (((double) chunk->samples[tmp]) / 32768.0); 
	}
		
	const std::vector<float> chunkToPredict(&chunk->fSamples[0], &chunk->fSamples[nrVADSamples]);
	
	// actual VAD processing
	sileroVadInst->predict(chunkToPredict);
		
	// 1 == active, 0 == not active, -1 == error
	chunk->state = (sileroVadInst->getTriggered() == true) ? VADState::ACTIVE : VADState::OFF;

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
bool VADWrapperSilero::analyze(bool hintShortAudio)
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
		case VADWrapperState::BUFFERING:
			findUtteranceStop(hintShortAudio);
			break;
		// utterance start and stop detected, duplicate data for the postbuf period
		case VADWrapperState::POSTBUF:
			// nothing to analyze, wait for postbuf data drained before going idle
			break;
	}
	
	// must use the more complex computation due to postbuffering (chunks in queue != available chunks)
	return (getAvailableChunks() == 0) ? true : false;
}

//////////////////////////////////////////////
unsigned int VADWrapperSilero::getAvailableChunks(void)
{
	switch (state)
	{
		case VADWrapperState::IDLE:
			// don't feed irrelevant silence to recognizer 
			return 0;
		case VADWrapperState::BUFFERING:
			// all chunks can be read
			return chunks.size();
		case VADWrapperState::POSTBUF:
			if (m_unbufferedStopChunksOffset > 0)
			{
				// existing chunks until the end of utterance was analyzed
				unsigned int availableChunks = m_unbufferedStopChunksOffset;
				// expected additional chunks, might not yet be present 
				availableChunks += m_audioPostBufferFrames;
				availableChunks = std::min(availableChunks, (unsigned int) chunks.size());
				// std::cout << "POSTBUF read UNbuffered: available=" << availableChunks << ", deque size=" << chunks.size() << "." << std::endl;
				return availableChunks;
			}
			else
			{
				// compute the offset from where the next chunk would be read
				// (buffered chunks are not erased)
				unsigned int nextChunkOffset = m_audioPostBufferFrames - m_bufferedStopChunksCountDown;
				
				// how many chunks are in buffer
				unsigned int availableChunksUtterance = (unsigned int) chunks.size();
				// but use no more than what belongs to the current utterance (max buffered)
				availableChunksUtterance = std::min(availableChunksUtterance, m_audioPostBufferFrames); 
					
				unsigned int announcedChunks;
				if (nextChunkOffset >= availableChunksUtterance)
				{
					// would like to read a chunk that is not yet in buffer
					announcedChunks = 0;	
				}
				else
				{
					// this many buffered chunks can be read
					announcedChunks = availableChunksUtterance - nextChunkOffset;					
				}
				// std::cout << "POSTBUF read buffered: offset=" << nextChunkOffset << ", deque size=" << chunks.size() << ", returning " << announcedChunks << "." << std::endl;
				return announcedChunks;
			}
	}
	
	assert(false);
	
	return 0;
}

//////////////////////////////////////////////
bool VADWrapperSilero::findUtteranceStart(void)
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
			// "best" case all frames are active frames, thus i can be one less than the min hystheresis 
			assert((i + 1) >= m_vadHystheresisFramesOn);
			
			state = VADWrapperState::BUFFERING;
			
			// handle the case where there is an immediate start
			if (i >= m_vadHystheresisFramesOn)
			{
				chunkUttStart = i - m_vadHystheresisFramesOn;
			}
			else
			{
				chunkUttStart = 0;
			}
			
			chunksAnalyzedStart = i;
			break;
		}
		// or: heavy toggling
		if ((numberActiveFrames > 0) && (numberToggles > vadMaxNrToggles))
		{
			// "best" case all frames were toggling, thus i can be one less than the min hystheresis 
			assert((i + 1) >= vadMaxNrToggles);
			
			state = VADWrapperState::BUFFERING;
			
			// handle the case where there is an immediate start
			if (i >= vadMaxNrToggles)
			{
				chunkUttStart = i - vadMaxNrToggles;
			}
			else
			{
				chunkUttStart = 0;	
			}
			chunksAnalyzedStart = i;
			break;
		}
		
		lastState = chunks[i]->state;
	}
	
	///////////////////////////////////////////////////
	// 2. remember properties of start chunk
	///////////////////////////////////////////////////
	
	if (state == VADWrapperState::BUFFERING)
	{
		frameCtrStart = chunks[chunkUttStart]->currFrameCtr;
		
		std::chrono::time_point<std::chrono::system_clock> timeStampStart = chunks[chunkUttStart]->currFrameTime;
	
		uStartTime   = std::chrono::duration_cast<std::chrono::seconds>(timeStampStart.time_since_epoch()).count();
		uStartTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeStampStart.time_since_epoch()).count() - (uStartTime * 1000);
		
		std::cout << "+++ Utterance start at chunk " << chunkUttStart << ", framectr " << frameCtrStart << std::endl; 
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
			chunksChopOffIdx = chunkUttStart - m_audioPreBufferFrames + 1;
		}
	}
	
	///////////////////////////////////////////////////
	// 4. chop off old chunks
	///////////////////////////////////////////////////
	
	if (chunksChopOffIdx > 0)
	{
		std::cout << "Erasing " << chunksChopOffIdx << " frames from buffer start." << std::endl;
		chunks.erase(chunks.begin(), chunks.begin() + chunksChopOffIdx);	
	}
	
	///////////////////////////////////////////////////
	// 5. return if start found
	///////////////////////////////////////////////////
	
	if (state == VADWrapperState::BUFFERING)
	{
		m_analyzeStopOffset = chunksAnalyzedStart - chunksChopOffIdx;
		return true;
	}
	else
	{
		// must be at max the amount of buffered frames
		assert(chunks.size() <= m_audioPreBufferFrames);
		return false;	
	}
}

//////////////////////////////////////////////
void VADWrapperSilero::findUtteranceStop(bool hintShortAudio)
{
	assert(state == VADWrapperState::BUFFERING);
	
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
			std::cout << "+++ Utterance stop at chunk " << i << ", framectr " << chunks[i]->currFrameCtr << std::endl; 
			
			state = VADWrapperState::POSTBUF;
			chunkUttEnd = i;
			break;
		}
	}
		
	///////////////////////////////////////////////////
	// 2. remember search props / assign utterance end props
	///////////////////////////////////////////////////
	if (state == VADWrapperState::BUFFERING)
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
		m_unbufferedStopChunksOffset  = (chunkUttEnd + 1);
		m_bufferedStopChunksCountDown = m_audioPostBufferFrames;
	}
}

//////////////////////////////////////////////
std::unique_ptr<VADFrame> VADWrapperSilero::getNextChunk(void)
{
	std::unique_ptr<VADFrame> chunk;
	
	assert(state != VADWrapperState::IDLE);
	assert(chunks.size() > 0);

	// supply the rest of the active part of the utterance (no copying)
	if ((state == VADWrapperState::BUFFERING) || ((state == VADWrapperState::POSTBUF) && (m_unbufferedStopChunksOffset > 0)))
	{
		// read & remove the first element
		chunk = std::move(chunks.front());
		
		// the invalid entry needs to be deleted
		chunks.pop_front();

		if (state == VADWrapperState::POSTBUF)
		{
			// std::cout << "Unbuffered stop chunk, frame ctr = " << chunk->currFrameCtr << "." << std::endl;
			
			m_unbufferedStopChunksOffset--;
		}
	}
	else
	{
		// copy the buffered chunks only, they shall be analyzed for a next possible start
		assert(m_bufferedStopChunksCountDown > 0);
		
		// std::cout << "Reading chunk " << (m_audioPostBufferFrames - m_bufferedStopChunksCountDown) << " from deque size " << chunks.size() << "." << std::endl;
		
		chunk = std::move(chunks.at(m_audioPostBufferFrames - m_bufferedStopChunksCountDown));
		chunks.erase(chunks.begin() + (m_audioPostBufferFrames - m_bufferedStopChunksCountDown));
		
		std::unique_ptr<VADFrame> chunkCopy = std::make_unique<VADFrame>(nrVADSamples);
		
		chunkCopy->state         = chunk->state;
		chunkCopy->currFrameCtr  = chunk->currFrameCtr;
		chunkCopy->currFrameTime = chunk->currFrameTime;

		memcpy(chunkCopy->samples, chunk->samples, (chunk->m_numberSamples * sizeof(short)));
#ifdef VAD_FRAME_CONVERT_FLOAT	
		memcpy(chunkCopy->fSamples, chunk->fSamples, (chunk->m_numberSamples * sizeof(float)));
#endif

		chunks.insert(chunks.begin() + (m_audioPostBufferFrames - m_bufferedStopChunksCountDown), std::move(chunkCopy));
		
		m_bufferedStopChunksCountDown--;
		if (m_bufferedStopChunksCountDown == 0)
		{
			state = VADWrapperState::IDLE;	
		}
	}
	
	// return the current (copied / moved) chunk
	return (chunk);
}

