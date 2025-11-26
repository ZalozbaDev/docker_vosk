
#include "doctest.h"

#include <RepetitionRemover.h>

#include <stddef.h>
#include <stdint.h>

#include <string>

TEST_CASE("passthrough mode")
{
	SUBCASE("check unmodified line") {
		Repetition rep;
		rep = RepetitionRemover::detectRepetitionByShift("abc");
		CHECK(rep.repetitions == 0);
	}

}

