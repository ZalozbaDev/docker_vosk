
#include "doctest.h"

#include <VoskRecognizer.h>

#include <stddef.h>
#include <stdint.h>

TEST_CASE("startup")
{	
	SUBCASE("run with 1 frame of samples") {
		VoskRecognizer *vosk = new VoskRecognizer(1, 48000, "dummyConfig");
	}


}
