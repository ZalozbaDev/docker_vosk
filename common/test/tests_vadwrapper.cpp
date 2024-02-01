
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

TEST_CASE("minimum number samples")
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

	/*
	SUBCASE("run with small samples") {
		int16_t buf[50];
		fill_buffer(buf, 50);
		wrapper->process(16000, buf, 50);
	}
	*/
	
	// delete wrapper;
}
