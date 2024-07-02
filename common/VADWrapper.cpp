
#include <VADWrapper.h>

#include <iostream>

#include <cassert>
#include <cstring>

#include <chrono>

//////////////////////////////////////////////
VADWrapper::VADWrapper(int aggressiveness, size_t frequencyHz)
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
	utteranceCurr  = -1;
	
	prebufCtrStart  = 0;
	prebufCtrToggle = 0;
	postBufCtrStop  = 0;
}

//////////////////////////////////////////////
VADWrapper::~VADWrapper(void)
{
	chunks.clear();
	
	WebRtcVad_Free(rtcVadInst);
}

//////////////////////////////////////////////
//
// process all data provided, plus leftover from previous call
// all data is VAD analyzed and stored in the "chunks" vector (leftover data is kept)
//
//////////////////////////////////////////////
int VADWrapper::process(int samplingFrequency, const int16_t* audio_frame, size_t frame_length, time_t startTime)
{
	int result, retVal;
	size_t frame_ptr;
	
	retVal = 0;
	frame_ptr = 0;
	
	while (leftOverSampleSize + (frame_length - frame_ptr) >= nrVADSamples)
	{
		std::unique_ptr<VADFrame<nrVADSamples>> chunk = std::make_unique<VADFrame<nrVADSamples>>();

		chunk->frameStartTime = startTime;
		
		// check and prepend leftover data
		if (leftOverSampleSize > 0)
		{
			int leftOverSampleBytes = leftOverSampleSize * sizeof(short);
			
			assert(leftOverSampleSize < nrVADSamples);
			
			memcpy(chunk->samples                               , leftOverSamples        , leftOverSampleBytes);
			memcpy(((char*)chunk->samples) + leftOverSampleBytes, audio_frame + frame_ptr, sizeof(chunk->samples) - leftOverSampleBytes);
			
			frame_ptr += nrVADSamples - leftOverSampleSize;
			leftOverSampleSize = 0;
		}
		else
		{
			memcpy(chunk->samples, audio_frame + frame_ptr, sizeof(chunk->samples));
			frame_ptr += nrVADSamples;
		}
		
		// actual VAD processing
		result = WebRtcVad_Process(rtcVadInst, samplingFrequency, chunk->samples, nrVADSamples);
		
		// log every frame result
		// std::cout << result;
		
		if (result == -1)
		{
			std::cout << "Error processing VAD data!" << std::endl;
			retVal = -1;
		}
		
		// 1 == active, 0 == not active, -1 == error
		chunk->state = (result == 1) ? VADState::ACTIVE : VADState::OFF;

#ifdef VAD_FRAME_CONVERT_FLOAT	
		if (result == 1)
		{
			// only when data will be used later, we need to convert to float for whisper
			for (unsigned int tmp = 0; tmp < nrVADSamples; tmp++)
			{
				chunk->fSamples[tmp] = (float) (((double) chunk->samples[tmp]) / 32768.0); 
			}
		}
#endif		
		
		chunks.push_back(std::move(chunk));
	}
	
	// finish logging VAD results
	// std::cout << std::endl;
	
	// remember leftover data
	if (frame_ptr < frame_length)
	{
		int leftOverSampleBytes = leftOverSampleSize * sizeof(short);
		
		assert(leftOverSampleSize + (frame_length - frame_ptr) < nrVADSamples);
		
		memcpy(((char*)leftOverSamples) + leftOverSampleBytes, audio_frame + frame_ptr, (frame_length - frame_ptr) * sizeof(short));
		leftOverSampleSize += frame_length - frame_ptr;
	}
	
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
		case VADWrapperState::IDLE:
			bool success;
			success = findUtteranceStart();
			if (success == true)
			{
				findUtteranceStop(hintShortAudio);
			}
			break;
		case VADWrapperState::INCOMPLETE:
			findUtteranceStop(hintShortAudio);
			break;
		case VADWrapperState::COMPLETE:
			// nothing to analyze as we have a complete utterance waiting to be fetched
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
		case VADWrapperState::INCOMPLETE:
			// all chunks can be read
			return chunks.size();
		case VADWrapperState::COMPLETE:
			// read chunks until the end of utterance was analyzed
			return (utteranceCurr + 1);
	}
	
	assert(false);
	
	return 0;
}

//////////////////////////////////////////////
std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> VADWrapper::getNextChunk(void)
{
	std::unique_ptr<VADFrame<VADWrapper::nrVADSamples>> chunk;
	
	assert(state != VADWrapperState::IDLE);
	assert(chunks.size() > 0);
	assert(utteranceCurr >= 0);
	
	utteranceCurr--;
	
	// reset to idle state if a complete utterance was fetched successfully
	if (state == VADWrapperState::COMPLETE)
	{
		if (utteranceCurr < 0)
		{
			std::cout << "VADWrapper::getNextChunk() resetting to IDLE after complete utterance was fetched" << std::endl;
			state = VADWrapperState::IDLE;
			utteranceCurr = -1;
			
			prebufCtrStart  = 0;
			prebufCtrToggle = 0;
			postBufCtrStop  = 0;
		}
	}
	
	// this moves the element to the local var but keeps an invalid (maybe null) entry in the deque
	chunk = std::move(chunks.front());
	
	// the invalid entry needs to be deleted
	chunks.pop_front();
	
	// and the value itself be returned
	return (chunk);
}

//////////////////////////////////////////////
bool VADWrapper::findUtteranceStart(void)
{
	assert(state == VADWrapperState::IDLE);
	assert(utteranceCurr < 0);
	
	// find possible beginning of utterance 
	for (unsigned int i = 0; i < chunks.size(); i++)
	{
		if (chunks[i]->state == VADState::ACTIVE)
		{
			prebufCtrStart++;
			prebufCtrToggle++;
		}
		else
		{
			if (prebufCtrStart > 0)
			{
				prebufCtrStart--;
				prebufCtrToggle++;
			}
		}

		if (prebufCtrToggle > 0)
		{
			std::cout << "Utterance possible start at " << i << " with toggleCtr " << prebufCtrToggle << "." << std::endl;
		}
		
		// did we find X active frames?
		if (prebufCtrStart == prebufVal)
		{
			// yes, chop off possible silence at beginning of vector
			unsigned int chopOff = (i > (2 * prebufVal)) ? (i - (2 * prebufVal) + 1) : 0;
			
			// std::cout << "Size b4=" << chunks.size() << ",chop off " << chopOff << std::endl;
			
			if (chopOff > 0)
			{
				chunks.erase(chunks.begin(), chunks.begin() + chopOff);	
			}
			
			// std::cout << "Size after=" << chunks.size()  << std::endl;
			
			// remember until where we analyzed (for faster search for end)
			utteranceCurr = i - chopOff;
			state = VADWrapperState::INCOMPLETE;

			std::chrono::time_point timeStampStart = std::chrono::system_clock::now();
			
			uStartTime   = std::chrono::duration_cast<std::chrono::seconds>(timeStampStart.time_since_epoch()).count();
			uStartTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeStampStart.time_since_epoch()).count() - (uStartTime * 1000);
			
			break;
		}
	}
	
	// if nothing found, we can trim stored elements
	if (state == VADWrapperState::IDLE)
	{
		if (chunks.size() > (prebufVal * 2))
		{
			// std::cout << "Trimming silence. Have " << chunks.size() << " chunks, reduce to " << (prebufVal * 2) << std::endl; 
			
			// delete everything but the last 10 frames
			chunks.erase(chunks.begin(), chunks.end() - (prebufVal * 2));

			// std::cout << "Chunks trimmed to " << chunks.size() << std::endl;
			
			assert(chunks.size() == (prebufVal * 2));
		}
		
		return false;
	}
	
	std::cout << "VADWrapper::findUtteranceStart() triggered new utterance!" << std::endl;
	
	return true;
}

//////////////////////////////////////////////
void VADWrapper::findUtteranceStop(bool hintShortAudio)
{
	unsigned int searchStart = 0;
	
	unsigned int maxPostbufVal = (hintShortAudio == true) ? postbufValShort : postbufValLong;
	
	assert(state == VADWrapperState::INCOMPLETE);
	
	if (utteranceCurr > 0)
	{
		searchStart = utteranceCurr;
	}
	
	// std::cout << "VADWrapper::findUtteranceStop() searching from " << searchStart << " to " << chunks.size() << std::endl; 
	
	// find possible end of utterance 
	for (unsigned int i = searchStart; i < chunks.size(); i++)
	{
		if (chunks[i]->state == VADState::OFF)
		{
			postBufCtrStop++;
		}
		else
		{
			postBufCtrStop = 0;	
		}
		
		// did we find X consecutive silent frames?
		if (postBufCtrStop == maxPostbufVal)
		{
			std::cout << "Utterance stop found at " << i << "." << std::endl; 
			
			// utterance stops right here
			utteranceCurr = i;
			state = VADWrapperState::COMPLETE;
			
			std::chrono::time_point timeStampStop = std::chrono::system_clock::now();
			
			uStopTime   = std::chrono::duration_cast<std::chrono::seconds>(timeStampStop.time_since_epoch()).count();
			uStopTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeStampStop.time_since_epoch()).count() - (uStopTime * 1000);
			
			break;
		}
	}

	// not complete yet, so remember how far we analyzed
	if (state == VADWrapperState::INCOMPLETE)
	{
		// continue at next (not yet existing) chunk
		utteranceCurr = chunks.size();
//		std::cout << "VADWrapper::findUtteranceStop() still accumulating, chunks=" << chunks.size() 
//		          << ", postBufCtr=" << postBufCtrStop 
//		          << ", continue at index=" << utteranceCurr << std::endl;
	}
	else
	{
		std::cout << "VADWrapper::findUtteranceStop() complete, chunks=" << chunks.size() << " and end is at " << utteranceCurr << std::endl; 
	}
}

