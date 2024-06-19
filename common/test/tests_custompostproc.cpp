
#include "doctest.h"

#include <CustomPostProc.h>

#include <stddef.h>
#include <stdint.h>

#include <string>

TEST_CASE("passthrough mode")
{
	CustomPostProc cpp(false, "");
	
	SUBCASE("check unmodified line") {
		CHECK(cpp.processLine("witajće K Nam,.-!/").compare("witajće K Nam,.-!/") == 0);
	}

}

TEST_CASE("correction mode without list")
{
	CustomPostProc cpp(true, "");

	SUBCASE("check chars to be removed") {
		CHECK(cpp.processLine("witajće, k: nam!").compare("witajće k nam") == 0);
	}
	
}

TEST_CASE("correction mode with list")
{
	CustomPostProc cpp(true, "replacement_list.txt");

	SUBCASE("check replacements somewhere in text") {
		CHECK(cpp.processLine("hrajer feliks ričel so!").compare("hrajer Feliks Ričel so") == 0);
	}
	
	SUBCASE("check replacements at beginning of line") {
		CHECK(cpp.processLine("ben boese njeda so,").compare("Ben Boese njeda so") == 0);
	}
	
	SUBCASE("check replacements at end of line") {
		CHECK(cpp.processLine("ben böse njeda so, ričel").compare("Ben Boese njeda so Ričel") == 0);
	}
	
}

