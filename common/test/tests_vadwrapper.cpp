
#include "doctest.h"

#include <VADWrapper.h>

#include <stddef.h>
#include <stdint.h>

#include "webrtc_vad_mock.h"

#include <ctime>
#include <chrono>

#include <iostream>

#include <sys/time.h>

void fill_buffer(int16_t* buf, size_t valOffset, size_t len)
{
	for (size_t index = 0; index < len; index++)
	{
		buf[index] = index + valOffset;	
	}
}

void processBuffer(VADWrapper &wrapper, int16_t * buf, std::uint64_t frameCtr = 0)
{
	wrapper.process(16000, buf, 160, frameCtr, std::chrono::system_clock::now());	
}

TEST_CASE("test utterance start/stop computations --> successful configuration")
{
	unsigned int audioPreBufferFrames  = 15;
	unsigned int audioPostBufferFrames = 15; 
	unsigned int vadHystheresisFramesOn = 5;
	unsigned int vadHystheresisFramesOff = 5;
	int vad_aggressiveness = 3;
	
	VADWrapper wrapper(vad_aggressiveness, 16000, audioPreBufferFrames, audioPostBufferFrames, vadHystheresisFramesOn, vadHystheresisFramesOff);
	
	SUBCASE("1. test normal start and stop computation with default pre- and postbuffer values, analysis only after all frames supplied") {
		int16_t buf[160];
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames));
	}
	
	SUBCASE("2. test normal start and stop computation with default pre- and postbuffer values, with frames missing during readout, check analyze(bool) return value") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int missingPostBufferFrames = 5;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.analyze(false) == true);
		CHECK(wrapper.getAvailableChunks() == 0);
		
		// supply missing postbuf frames
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == missingPostBufferFrames);
		
		// try to read out all frames
		availableFrameCtr = missingPostBufferFrames;
		for (unsigned int i = 0; i < missingPostBufferFrames; i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
		
	}
	
	SUBCASE("3. test normal start and stop computation with default pre- and postbuffer values, analysis after each step, check analyze(bool) return value") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == true);
		// no frames announced when idle
		CHECK(wrapper.getAvailableChunks() == 0);
		
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		// announce all frames incl prebuffer
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff));

		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("4. test limit of prebuffer frames, analysis only after all frames supplied") {
		unsigned int skippedEmptyFrames = 23;
		int16_t buf[160];
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < (audioPreBufferFrames + skippedEmptyFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = (audioPreBufferFrames + skippedEmptyFrames); i < (audioPreBufferFrames + skippedEmptyFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// excess frames removed
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
	}
	
	SUBCASE("5. test limit of prebuffer frames, analysis after each step") {
		unsigned int skippedEmptyFrames = 17;
		std::uint64_t frameCtr = 0;
		int16_t buf[160];
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < (audioPreBufferFrames + skippedEmptyFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = (audioPreBufferFrames + skippedEmptyFrames); i < (audioPreBufferFrames + skippedEmptyFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// excess frames removed
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i + skippedEmptyFrames);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("6. test postbuffer logic with second utterance directly after postbuffer, read-out all before filling 2nd utterance") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);

		// std::cout << "adding 2nd utterance now!" << std::endl;
		
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = 0; i < vadHystheresisFramesOn; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		
		wrapper.analyze(false);
		// must indicate prebuf+active frames available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames and check frame order / copying
		availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("7. test postbuffer logic with second utterance shortly after postbuffer, read-out all before filling 2nd utterance") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int firstUttLength = 35;
		unsigned int secondUttOffset = 7;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		
		std::cout << "last frame counter for first utterance=" << (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames) << "." << std::endl;

		// std::cout << "adding 2nd utterance now!" << std::endl;
		
		unsigned int frameCounterOffset2ndUtt = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		frameCounterOffset2ndUtt -= (audioPreBufferFrames - secondUttOffset);
		
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < secondUttOffset; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = secondUttOffset; i < (secondUttOffset + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		
		wrapper.analyze(false);
		// must indicate prebuf+active frames available (offset skipped)
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames and check frame order / copying
		availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + frameCounterOffset2ndUtt));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("8. test postbuffer logic with second utterance with distance from postbuffer, read-out all before filling 2nd utterance") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int firstUttLength = 35;
		unsigned int secondUttOffset = 3 * audioPostBufferFrames;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		
		std::cout << "last frame counter for first utterance=" << (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames) << "." << std::endl;

		// std::cout << "adding 2nd utterance now!" << std::endl;
		
		unsigned int frameCounterOffset2ndUtt = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		frameCounterOffset2ndUtt += secondUttOffset - audioPreBufferFrames;
		
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < secondUttOffset; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = secondUttOffset; i < (secondUttOffset + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		
		wrapper.analyze(false);
		// must indicate prebuf+active frames available (offset skipped)
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames and check frame order / copying
		availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + frameCounterOffset2ndUtt));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("9. test postbuffer logic with second utterance shortly after postbuffer, analyze and read-out after 2nd utterance") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int firstUttLength = 35;
		unsigned int secondUttOffset = 7;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}

		std::cout << "last frame counter for first utterance=" << (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames) << "." << std::endl;

		// std::cout << "adding 2nd utterance now!" << std::endl;
		
		unsigned int frameCounterOffset2ndUtt = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		frameCounterOffset2ndUtt -= (audioPreBufferFrames - secondUttOffset);
		
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < secondUttOffset; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = secondUttOffset; i < (secondUttOffset + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}

		/////////////////////////////
		// analyze 1st utt
		/////////////////////////////
		CHECK(wrapper.analyze(false) == false);
		
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		
		// std::cout << "Frames provided=" << frameCtr << ", chunks available=" << wrapper.getAvailableChunks() << ", available frames ctr=" << availableFrameCtr << "." << std::endl;
		
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
			
			// std::cout << "Curr framectr=" << i << ", available framectr=" << availableFrameCtr << ", available chunks=" << wrapper.getAvailableChunks() << "." << std::endl;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		
		/////////////////////////////
		// analyze 2nd utt
		/////////////////////////////
		CHECK(wrapper.analyze(false) == false);
		
		// must indicate prebuf+active frames available (offset skipped)
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames and check frame order / copying
		availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + frameCounterOffset2ndUtt));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("10. test toggling for start computation with default pre- and postbuffer values") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int nrToggles = 5;
		unsigned int firstUttLength = 23;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// no frames announced when idle
		CHECK(wrapper.getAvailableChunks() == 0);
		
		for (unsigned int k = audioPreBufferFrames; k < (audioPreBufferFrames + nrToggles * 2); k += 2)
		{
			WebRtcVad_Mock_set_result(1);
			fill_buffer(buf, k * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
			WebRtcVad_Mock_set_result(0);
			fill_buffer(buf, (k + 1) * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}			
		
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = (audioPreBufferFrames + nrToggles * 2); i < (audioPreBufferFrames + nrToggles * 2 + firstUttLength); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + nrToggles * 2 + firstUttLength); i < (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		for (unsigned int i = (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff); i < (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}

		CHECK(wrapper.analyze(false) == false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	

}

TEST_CASE("test utterance start/stop computations --> failed configuration")
{
	unsigned int audioPreBufferFrames  = 3;
	unsigned int audioPostBufferFrames = 3; 
	unsigned int vadHystheresisFramesOn = 5;
	unsigned int vadHystheresisFramesOff = 10;
	int vad_aggressiveness = 3;
	
	// FIXME: error with these settings VAD is never turned off!!!
	
	VADWrapper wrapper(vad_aggressiveness, 16000, audioPreBufferFrames, audioPostBufferFrames, vadHystheresisFramesOn, vadHystheresisFramesOff);
	
	SUBCASE("1. test normal start and stop computation with default pre- and postbuffer values, analysis only after all frames supplied") {
		int16_t buf[160];
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames));
	}
	
	SUBCASE("2. test normal start and stop computation with default pre- and postbuffer values, with frames missing during readout, check analyze(bool) return value") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int missingPostBufferFrames = 5;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.analyze(false) == true);
		CHECK(wrapper.getAvailableChunks() == 0);
		
		// supply missing postbuf frames
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == missingPostBufferFrames);
		
		// try to read out all frames
		availableFrameCtr = missingPostBufferFrames;
		for (unsigned int i = 0; i < missingPostBufferFrames; i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames - missingPostBufferFrames));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
		
	}
	
	SUBCASE("3. test normal start and stop computation with default pre- and postbuffer values, analysis after each step, check analyze(bool) return value") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == true);
		// no frames announced when idle
		CHECK(wrapper.getAvailableChunks() == 0);
		
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		// announce all frames incl prebuffer
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff));

		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		CHECK(wrapper.analyze(false) == false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("4. test limit of prebuffer frames, analysis only after all frames supplied") {
		unsigned int skippedEmptyFrames = 23;
		int16_t buf[160];
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < (audioPreBufferFrames + skippedEmptyFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = (audioPreBufferFrames + skippedEmptyFrames); i < (audioPreBufferFrames + skippedEmptyFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// excess frames removed
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
	}
	
	SUBCASE("5. test limit of prebuffer frames, analysis after each step") {
		unsigned int skippedEmptyFrames = 17;
		std::uint64_t frameCtr = 0;
		int16_t buf[160];
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < (audioPreBufferFrames + skippedEmptyFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = (audioPreBufferFrames + skippedEmptyFrames); i < (audioPreBufferFrames + skippedEmptyFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// excess frames removed
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i + skippedEmptyFrames);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("6. test postbuffer logic with second utterance directly after postbuffer, read-out all before filling 2nd utterance") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);

		// std::cout << "adding 2nd utterance now!" << std::endl;
		
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = 0; i < vadHystheresisFramesOn; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		
		wrapper.analyze(false);
		// must indicate prebuf+active frames available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames and check frame order / copying
		availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("7. test postbuffer logic with second utterance shortly after postbuffer, read-out all before filling 2nd utterance") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int firstUttLength = 35;
		unsigned int secondUttOffset = 7;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		
		std::cout << "last frame counter for first utterance=" << (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames) << "." << std::endl;

		// std::cout << "adding 2nd utterance now!" << std::endl;
		
		unsigned int frameCounterOffset2ndUtt = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		frameCounterOffset2ndUtt -= (audioPreBufferFrames - secondUttOffset);
		
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < secondUttOffset; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = secondUttOffset; i < (secondUttOffset + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		
		wrapper.analyze(false);
		// must indicate prebuf+active frames available (offset skipped)
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames and check frame order / copying
		availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + frameCounterOffset2ndUtt));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("8. test postbuffer logic with second utterance with distance from postbuffer, read-out all before filling 2nd utterance") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int firstUttLength = 35;
		unsigned int secondUttOffset = 3 * audioPostBufferFrames;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		
		std::cout << "last frame counter for first utterance=" << (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames) << "." << std::endl;

		// std::cout << "adding 2nd utterance now!" << std::endl;
		
		unsigned int frameCounterOffset2ndUtt = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		frameCounterOffset2ndUtt += secondUttOffset - audioPreBufferFrames;
		
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < secondUttOffset; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = secondUttOffset; i < (secondUttOffset + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		
		wrapper.analyze(false);
		// must indicate prebuf+active frames available (offset skipped)
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames and check frame order / copying
		availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + frameCounterOffset2ndUtt));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("9. test postbuffer logic with second utterance shortly after postbuffer, analyze and read-out after 2nd utterance") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int firstUttLength = 35;
		unsigned int secondUttOffset = 7;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength); i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}

		std::cout << "last frame counter for first utterance=" << (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames) << "." << std::endl;

		// std::cout << "adding 2nd utterance now!" << std::endl;
		
		unsigned int frameCounterOffset2ndUtt = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		frameCounterOffset2ndUtt -= (audioPreBufferFrames - secondUttOffset);
		
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < secondUttOffset; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = secondUttOffset; i < (secondUttOffset + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}

		/////////////////////////////
		// analyze 1st utt
		/////////////////////////////
		CHECK(wrapper.analyze(false) == false);
		
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		
		// std::cout << "Frames provided=" << frameCtr << ", chunks available=" << wrapper.getAvailableChunks() << ", available frames ctr=" << availableFrameCtr << "." << std::endl;
		
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
			
			// std::cout << "Curr framectr=" << i << ", available framectr=" << availableFrameCtr << ", available chunks=" << wrapper.getAvailableChunks() << "." << std::endl;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		
		/////////////////////////////
		// analyze 2nd utt
		/////////////////////////////
		CHECK(wrapper.analyze(false) == false);
		
		// must indicate prebuf+active frames available (offset skipped)
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		// try to read out all frames and check frame order / copying
		availableFrameCtr = audioPreBufferFrames + vadHystheresisFramesOn;
		for (unsigned int i = 0; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == (i + frameCounterOffset2ndUtt));
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("10. test toggling for start computation with default pre- and postbuffer values") {
		int16_t buf[160];
		std::uint64_t frameCtr = 0;
		unsigned int nrToggles = 5;
		unsigned int firstUttLength = 23;
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		wrapper.analyze(false);
		// no frames announced when idle
		CHECK(wrapper.getAvailableChunks() == 0);
		
		for (unsigned int k = audioPreBufferFrames; k < (audioPreBufferFrames + nrToggles * 2); k += 2)
		{
			WebRtcVad_Mock_set_result(1);
			fill_buffer(buf, k * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
			WebRtcVad_Mock_set_result(0);
			fill_buffer(buf, (k + 1) * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}			
		
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = (audioPreBufferFrames + nrToggles * 2); i < (audioPreBufferFrames + nrToggles * 2 + firstUttLength); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + nrToggles * 2 + firstUttLength); i < (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}
		for (unsigned int i = (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff); i < (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf, frameCtr++);
		}

		CHECK(wrapper.analyze(false) == false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames));
		
		// try to read out all frames
		std::unique_ptr<VADFrame> frame;
		unsigned int availableFrameCtr = audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames;
		for (unsigned int i = 0; i < (audioPreBufferFrames + nrToggles * 2 + firstUttLength + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			CHECK(wrapper.getAvailableChunks() == availableFrameCtr);
			frame = wrapper.getNextChunk();
			CHECK(frame->currFrameCtr == i);
			availableFrameCtr--;
		}
		CHECK(wrapper.getAvailableChunks() == 0);
		CHECK(wrapper.analyze(false) == true);
	}
	

}

TEST_CASE("test timestamps")
{
	VADWrapper wrapper(3, 16000);
	
	SUBCASE("1. check C/C++ computations") {
		time_t stamp = time(NULL);
		std::chrono::time_point newStamp = std::chrono::system_clock::now();
		
		std::cout << "Seconds since epoch" << std::endl;
		std::cout << "C   impl: " << stamp << std::endl;
		std::cout << "C++ impl: " << std::chrono::duration_cast<std::chrono::seconds>(newStamp.time_since_epoch()).count() << std::endl;
		
		struct timeval tp;
		gettimeofday(&tp, NULL);
		
		std::chrono::time_point newMsStamp = std::chrono::system_clock::now();
		
		long int stamp_s      = tp.tv_sec;
		long int stamp_millis = tp.tv_usec / 1000;

		long int newStamp_s  = std::chrono::duration_cast<std::chrono::seconds>(newMsStamp.time_since_epoch()).count();
		long int newStamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(newMsStamp.time_since_epoch()).count() - (newStamp_s * 1000);
		
		std::cout << "Seconds and ms since epoch" << std::endl;
		std::cout << "C   impl: " << stamp_s << " s, " << stamp_millis << " ms." << std::endl;
		std::cout << "C++ impl: " << newStamp_s << " s, " << newStamp_ms << " ms." << std::endl;
		
	}
	
}

TEST_CASE("test different VAD aggressiveness (simulated)")
{
	unsigned int audioPreBufferFrames  = 15;
	unsigned int audioPostBufferFrames = 15; 
	unsigned int vadHystheresisFramesOn = 5;
	unsigned int vadHystheresisFramesOff = 5;
	int vad_aggressiveness = 1;
	
	VADWrapper wrapper(vad_aggressiveness, 16000, audioPreBufferFrames, audioPostBufferFrames, vadHystheresisFramesOn, vadHystheresisFramesOff);
	
	SUBCASE("1. test with all frames active from beginning") {
		int16_t buf[160];
		
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames));
	}
}

TEST_CASE("test handling with empty buffer")
{
	unsigned int audioPreBufferFrames  = 15;
	unsigned int audioPostBufferFrames = 15; 
	unsigned int vadHystheresisFramesOn = 5;
	unsigned int vadHystheresisFramesOff = 5;
	int vad_aggressiveness = 1;
	
	VADWrapper wrapper(vad_aggressiveness, 16000, audioPreBufferFrames, audioPostBufferFrames, vadHystheresisFramesOn, vadHystheresisFramesOff);
	
	SUBCASE("1. test with inactive frames") {
		int16_t buf[160];
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < 8; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		CHECK(wrapper.analyze(false) == true);
	}
	
	SUBCASE("2. test with active frames") {
		int16_t buf[160];
		
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = 0; i < 8; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		CHECK(wrapper.analyze(false) == false);
	}
}
		


// leftover samples functionality removed from VAD wrapper
/*
TEST_CASE("test handling of leftover samples")
{
	VADWrapper wrapper(3, 16000);
	
	SUBCASE("run with 1 frame of samples") {
		int16_t buf[160];
		fill_buffer(buf, 0, 160);
		wrapper.process(16000, buf, 160);
		CHECK(wrapper.getLeftOverSampleSize() == 0);
	}

	SUBCASE("run with 1 frame and 35 additional samples") {
		int16_t buf[195];
		fill_buffer(buf, 0, 195);
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		wrapper.process(16000, buf, 195);
		CHECK(wrapper.getLeftOverSampleSize() == 35);
	}

	SUBCASE("supply additional samples to make full frame") {
		int16_t buf1[195];
		fill_buffer(buf1, 0, 195);
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		wrapper.process(16000, buf1, 195);
		CHECK(wrapper.getLeftOverSampleSize() == 35);

		int16_t buf2[285];
		fill_buffer(buf2, 195, 285);
		wrapper.process(16000, buf2, 285);
		CHECK(wrapper.getLeftOverSampleSize() == 0);
	}

	SUBCASE("run with small samples") {
		int16_t buf[50];
		fill_buffer(buf, 0, 50);
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		wrapper.process(16000, buf, 50);
		CHECK(wrapper.getLeftOverSampleSize() == 50);
	}
	
	SUBCASE("accumulate samples until frame full") {
		int16_t buf[40];
		fill_buffer(buf, 0, 40);
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		wrapper.process(16000, buf, 40);
		CHECK(wrapper.getLeftOverSampleSize() == 40);
		
		fill_buffer(buf, 40, 40);
		wrapper.process(16000, buf, 40);
		CHECK(wrapper.getLeftOverSampleSize() == 80);

		fill_buffer(buf, 80, 40);
		wrapper.process(16000, buf, 40);
		CHECK(wrapper.getLeftOverSampleSize() == 120);

		fill_buffer(buf, 120, 40);
		wrapper.process(16000, buf, 40);
		CHECK(wrapper.getLeftOverSampleSize() == 0);
	}
	
	SUBCASE("check corner case one sample too few and one too much for full frame") {
		int16_t buf1[159];
		fill_buffer(buf1, 0, 159);
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		wrapper.process(16000, buf1, 159);
		CHECK(wrapper.getLeftOverSampleSize() == 159);

		int16_t buf2[2];
		fill_buffer(buf2, 159, 2);
		wrapper.process(16000, buf2, 2);
		CHECK(wrapper.getLeftOverSampleSize() == 1);

		int16_t buf3[159];
		fill_buffer(buf3, 161, 159);
		wrapper.process(16000, buf3, 159);
		CHECK(wrapper.getLeftOverSampleSize() == 0);
	}


}
*/

