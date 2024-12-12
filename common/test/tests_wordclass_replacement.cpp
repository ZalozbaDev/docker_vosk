
#include "doctest.h"

#include <WordClassPostProc.h>

#include <stddef.h>
#include <stdint.h>

#include <string>

TEST_CASE("test")
{
	WordClassPostProc wcpp(false);
	
	SUBCASE("check correct line to not be modified") {
		CHECK(wcpp.processLine("witajće k nam") == "witajće k nam");
	}

}

