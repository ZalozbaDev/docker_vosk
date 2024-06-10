
#include "doctest.h"

#include <VoskRecognizer.h>

#include <stddef.h>
#include <stdint.h>

void fill_buffer(int16_t* buf, size_t valOffset, size_t len)
{
	for (size_t index = 0; index < len; index++)
	{
		buf[index] = (index / 3) + valOffset;	
	}
}

TEST_CASE("partial and full result format output options")
{	
	SUBCASE("simple partial result (empty)") {
		char dummyData[16000];
		VoskRecognizer vosk(1, 48000, "dummyConfig");
		vosk.setDetailedResult(false);
		vosk.acceptWaveform(dummyData, sizeof(dummyData));
		CHECK(std::string(vosk.getPartialResult()).compare("{ \"partial\" : \"\" }") == 0);
	}

	SUBCASE("detailed partial result (empty), recognizer inactive") {
		char dummyData[16000] = {0};
		VoskRecognizer vosk(1, 48000, "dummyConfig");
		vosk.setDetailedResult(true);
		vosk.acceptWaveform(dummyData, sizeof(dummyData));
		CHECK(std::string(vosk.getPartialResult()).compare("{ \"partial\" : \"\", \"listen\" : \"false\" }") == 0);
	}

	SUBCASE("detailed partial result (empty), recognizer active") {
		char dummyData[48000] = {127};
		VoskRecognizer vosk(1, 48000, "dummyConfig");
		vosk.setDetailedResult(true);
		vosk.acceptWaveform(dummyData, sizeof(dummyData));
		fill_buffer((int16_t *) &dummyData, 0, sizeof(dummyData) / 2);
		WebRtcVad_Mock_set_result(1);
		vosk.acceptWaveform(dummyData, sizeof(dummyData) / 2);
		CHECK(std::string(vosk.getPartialResult()).compare("{ \"partial\" : \"\", \"listen\" : \"true\" }") == 0);
	}


}
