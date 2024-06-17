
#include "doctest.h"

#include <CustomPostProc.h>

#include <stddef.h>
#include <stdint.h>

#include <string>

TEST_CASE("passthrough mode")
{
	CustomPostProc cpp(false);
	
	SUBCASE("check unmodified line") {
		CHECK(cpp.processLine("witajće K Nam,.-!/").compare("witajće K Nam,.-!/") == 0);
	}

}

TEST_CASE("correction mode")
{
	CustomPostProc cpp(true);

}

