
#include "doctest.h"

#include <VADWrapper.h>

#include <stddef.h>
#include <stdint.h>

#include "webrtc_vad_mock.h"

void fill_buffer(int16_t* buf, size_t valOffset, size_t len)
{
	for (size_t index = 0; index < len; index++)
	{
		buf[index] = index + valOffset;	
	}
}

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

TEST_CASE("test utterance start/stop computations")
{
	VADWrapper wrapper(3, 16000);
	
	SUBCASE("check normal start/stop behaviour") {
		int16_t buf[160];
		WebRtcVad_Mock_set_result(0);
		for (int i = 0; i < 10; i++)
		{
			fill_buffer(buf, i * 160, 160);
			wrapper.process(16000, buf, 160);
		}
		WebRtcVad_Mock_set_result(1);
		for (int i = 10; i < 20; i++)
		{
			fill_buffer(buf, i * 160, 160);
			wrapper.process(16000, buf, 160);
		}
		WebRtcVad_Mock_set_result(0);
		for (int i = 20; i < 40; i++)
		{
			fill_buffer(buf, i * 160, 160);
			wrapper.process(16000, buf, 160);
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
			wrapper.process(16000, buf, 160);
		}
		WebRtcVad_Mock_set_result(1);
		for (int i = 10; i < 20; i++)
		{
			fill_buffer(buf, i * 160, 160);
			wrapper.process(16000, buf, 160);
		}
		WebRtcVad_Mock_set_result(0);
		for (int i = 20; i < 40; i++)
		{
			fill_buffer(buf, i * 160, 160);
			wrapper.process(16000, buf, 160);
		}
		wrapper.analyze(true);
		// 10 active, 5 pre, 10 post
		CHECK(wrapper.getAvailableChunks() == 25);
	}
	
}
