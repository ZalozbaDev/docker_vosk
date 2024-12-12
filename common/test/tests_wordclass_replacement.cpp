
#include "doctest.h"

#include <WordClassPostProc.h>

#include <stddef.h>
#include <stdint.h>

#include <string>

TEST_CASE("test passthrough mode")
{
	WordClassPostProc wcpp(false);
	
	SUBCASE("check correct line to not be modified") {
		CHECK(wcpp.processLine("witajće k nam") == "witajće k nam");
	}

}

TEST_CASE("test currency parser")
{
	WordClassPostProc wcpp(true);
	
	SUBCASE("check correct line to not be modified") {
		CHECK(wcpp.processLine("witajće k nam") == "witajće k nam");
	}

	SUBCASE("check currency with only euro") {
		CHECK(wcpp.processLine("je so nahromadźiło <CURRENCY>+200+7+50</CURRENCY> za") == "je so nahromadźiło 257.00€ za");
	}

}

