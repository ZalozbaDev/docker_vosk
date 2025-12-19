
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

	///////////////////////////////////////
	//
	// replacements start, middle, end
	//
	///////////////////////////////////////
	SUBCASE("check replacements somewhere in text") {
		CHECK(cpp.processLine("hrajer feliks ričel so!").compare("hrajer Feliks Ričel so") == 0);
	}
	
	SUBCASE("check replacements at beginning of line") {
		CHECK(cpp.processLine("ben boese njeda so,").compare("Ben Boese njeda so") == 0);
	}
	
	SUBCASE("check replacements at end of line") {
		CHECK(cpp.processLine("ben böse njeda so, ričel").compare("Ben Boese njeda so Ričel") == 0);
	}
	
	///////////////////////////////////////
	//
	// no replacements start, middle, end
	//
	///////////////////////////////////////
	SUBCASE("check no replacement if word does not match exactly") {
		CHECK(cpp.processLine("benej böseu njeda so, ričel").compare("benej böseu njeda so Ričel") == 0);
	}
	
	SUBCASE("check no replacement at beginning of line when match not at word start") {
		CHECK(cpp.processLine("aben boese njeda so,").compare("aben Boese njeda so") == 0);
	}
	
	SUBCASE("check no replacement somewhere in text when match not at word start") {
		CHECK(cpp.processLine("ben boese alričel njeda so,").compare("Ben Boese alričel njeda so") == 0);
	}
	
	SUBCASE("check no replacement of last word when match not at word start") {
		CHECK(cpp.processLine("ben boese ričel njeda so, alričel").compare("Ben Boese Ričel njeda so alričel") == 0);
	}
	
	///////////////////////////////////////
	//
	// suffix length
	//
	///////////////////////////////////////
	SUBCASE("check no replacement of last word when match not at word start") {
		CHECK(cpp.processLine("chróšćic chróšćic chróšćicy chróšćicej chróšćicy").compare("Chróšćic Chróšćic Chróšćicy chróšćicej Chróšćicy") == 0);
	}
	
	///////////////////////////////////////
	//
	// replacee with space
	//
	///////////////////////////////////////
	SUBCASE("check no replacement of last word when match not at word start") {
		CHECK(cpp.processLine("to je sven erik lehmann tule").compare("to je Sven Erik Lehmann tule") == 0);
	}
	
}

TEST_CASE("case modification")
{
	CustomPostProc cpp(true, "replacement_list.txt", true);
	
	SUBCASE("check lower casing of buffer and then replacements") {
		CHECK(cpp.processLine("HRAJER FELIKS RIČEL SO!").compare("hrajer Feliks Ričel so") == 0);
	}
	
}

