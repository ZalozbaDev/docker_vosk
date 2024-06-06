
#include "doctest.h"

#include <HunspellPostProc.h>

#include <stddef.h>
#include <stdint.h>

#include <string>

TEST_CASE("passthrough mode")
{
	HunspellPostProc hpp("", "", "");
	
	SUBCASE("check correct line") {
		CHECK(hpp.processLine("witajće k nam") == "witajće k nam");
	}

}

TEST_CASE("dictionary mode")
{
	HunspellPostProc hpp("./hsb_DE_soblex_w8_3.09.03.aff", "./hsb_DE_soblex_w8_3.09.03.dic", "");
	
	SUBCASE("check correct line") {
		CHECK(hpp.processLine("witajće k nam").compare("witajće k nam") == 0);
	}

	SUBCASE("check incorrect result") {
		CHECK(hpp.processLine("witajće k nam").compare("witajće k num") != 0);
	}

	SUBCASE("check corrected result 1 spelling mistake") {
		CHECK(hpp.processLine("witajće k num").compare("witajće k nim") == 0);
	}

	SUBCASE("check corrected result 1 spelling mistake with case mixing") {
		CHECK(hpp.processLine("WiTaJćE k NuM").compare("Witaj Će k Nur") == 0);
	}

}

