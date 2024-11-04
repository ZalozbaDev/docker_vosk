
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

void processBuffer(VADWrapper &wrapper, int16_t * buf)
{
	wrapper.process(16000, buf, 160, 0, std::chrono::system_clock::now());	
}

TEST_CASE("test utterance start/stop computations")
{
	unsigned int audioPreBufferFrames  = 15;
	unsigned int audioPostBufferFrames = 15; 
	unsigned int vadHystheresisFramesOn = 5;
	unsigned int vadHystheresisFramesOff = 5;
	
	VADWrapper wrapper(3, 16000, audioPreBufferFrames, audioPostBufferFrames, vadHystheresisFramesOn, vadHystheresisFramesOff);
	
	SUBCASE("test normal start and stop computation with default pre- and postbuffer values, analysis only after all frames supplied") {
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
	
	SUBCASE("test normal start and stop computation with default pre- and postbuffer values, analysis after each step") {
		int16_t buf[160];
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < audioPreBufferFrames; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// no frames announced when idle
		CHECK(wrapper.getAvailableChunks() == 0);
		
		WebRtcVad_Mock_set_result(1);
		for (unsigned int i = audioPreBufferFrames; i < (audioPreBufferFrames + vadHystheresisFramesOn); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// announce all frames incl prebuffer
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn));
		
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff));

		for (unsigned int i = (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff); i < (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// must indicate the whole buffer available
		CHECK(wrapper.getAvailableChunks() == (audioPreBufferFrames + vadHystheresisFramesOn + vadHystheresisFramesOff + audioPostBufferFrames));
	}
	
	SUBCASE("test limit of prebuffer frames, analysis only after all frames supplied") {
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
	
	SUBCASE("test limit of prebuffer frames, analysis after each step") {
		unsigned int skippedEmptyFrames = 17;
		int16_t buf[160];
		WebRtcVad_Mock_set_result(0);
		for (unsigned int i = 0; i < (audioPreBufferFrames + skippedEmptyFrames); i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
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
	
	
	
	/*
	
	SUBCASE("check normal start/stop behaviour") {
		int16_t buf[160];
		WebRtcVad_Mock_set_result(0);
		for (int i = 0; i < 10; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (int i = 10; i < 20; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(0);
		for (int i = 20; i < 40; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// 10 active, 5 pre, 5 post
		CHECK(wrapper.getAvailableChunks() == 20);
	}
	
	SUBCASE("check start/stop behaviour with short hint set") {
		int16_t buf[160];
		WebRtcVad_Mock_set_result(0);
		for (int i = 0; i < 10; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (int i = 10; i < 20; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(0);
		for (int i = 20; i < 40; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(true);
		// 10 active, 5 pre, 10 post
		CHECK(wrapper.getAvailableChunks() == 25);
	}
	
	SUBCASE("check start/stop behaviour with distributed anayze runs") {
		int16_t buf[160];
		WebRtcVad_Mock_set_result(0);
		for (int i = 0; i < 10; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (int i = 10; i < 20; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		WebRtcVad_Mock_set_result(0);
		for (int i = 20; i < 23; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		for (int i = 23; i < 40; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// 10 active, 5 pre, 5 post
		CHECK(wrapper.getAvailableChunks() == 20);
		
		// check that the next run is corect again
		WebRtcVad_Mock_reset(wrapper.getRtcVadInst());
		for (int i = 0; i < 10; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(1);
		for (int i = 10; i < 20; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		WebRtcVad_Mock_set_result(0);
		for (int i = 20; i < 40; i++)
		{
			fill_buffer(buf, i * 160, 160);
			processBuffer(wrapper, buf);
		}
		wrapper.analyze(false);
		// 10 active, 5 pre, 5 post
		CHECK(wrapper.getAvailableChunks() == 20);		
	}
	
	*/
	

}

TEST_CASE("test timestamps")
{
	VADWrapper wrapper(3, 16000);
	
	SUBCASE("check C/C++ computations") {
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

