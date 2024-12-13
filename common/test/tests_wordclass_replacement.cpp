
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

	SUBCASE("check currency with euro and cent, multiplier for thousand") {
		CHECK(wcpp.processLine("je so nahromadźiło <CURRENCY>+(+2)*1000+100+5+0+(+8+70)/100</CURRENCY> za") == "je so nahromadźiło 2105.78€ za");
	}

	SUBCASE("check currency with euro and cent, no multiplier for thousand") {
		CHECK(wcpp.processLine("je so nahromadźiło <CURRENCY>+1000+500+1+50+0+0+(+2+90)/100</CURRENCY> za") == "je so nahromadźiło 1551.92€ za");
	}

	SUBCASE("check currency with an invalid expression and check for error handling") {
		CHECK(wcpp.processLine("je so nahromadźiło <CURRENCY>+1000+huhuhu+1+50+0+0+(+2+90)/100</CURRENCY> za") == "je so nahromadźiło ???€ za");
	}

}

TEST_CASE("test percentage parser")
{
	WordClassPostProc wcpp(true);
	
	SUBCASE("check percentage without fraction") {
		CHECK(wcpp.processLine("wobdźělenja <PERCENT>+3+60</PERCENT> při wólbach") == "wobdźělenja 63.0% při wólbach");
	}

	SUBCASE("check percentage with fraction") {
		CHECK(wcpp.processLine("wobdźělenja <PERCENT>+7+20+(+5)/10</PERCENT> při wólbach") == "wobdźělenja 27.5% při wólbach");
	}

	SUBCASE("check percentage with error") {
		CHECK(wcpp.processLine("wobdźělenja <PERCENT>ohrmenno</PERCENT> při wólbach") == "wobdźělenja ???% při wólbach");
	}

}

