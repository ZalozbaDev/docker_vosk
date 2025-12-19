
#include "doctest.h"

#include <CustomPostProc.h>

#include <stddef.h>
#include <stdint.h>

#include <string>

TEST_CASE("simple tests")
{
	CustomPostProc cpp(false, "", false);
	
	SUBCASE("check simple sanitizing") {
		CHECK(cpp.sanitizeWord("witajće") == "witajće");
		CHECK(cpp.sanitizeWord("K") == "K");
		CHECK(cpp.sanitizeWord("Nam,.-!") == "Nam");
		
		// test invalid multibyte sequence (replaced with U+FFFD / see below for UTF-8 notation)
		CHECK(cpp.sanitizeWord("lubi\xF0\xA4\xAD") == "lubi\xEF\xBF\xBD");

		// check space removal
		CHECK(cpp.sanitizeWord(" witajće") == "witajće");
		CHECK(cpp.sanitizeWord("  witajće") == "witajće");
		CHECK(cpp.sanitizeWord("witajće ") == "witajće");
		CHECK(cpp.sanitizeWord("witajće  ") == "witajće");
		CHECK(cpp.sanitizeWord(" w  i    t a j ć e") == "witajće");
		
		// check interpunction removal
		CHECK(cpp.sanitizeWord("witajće,") == "witajće");
		CHECK(cpp.sanitizeWord("k:")       == "k");
		CHECK(cpp.sanitizeWord("nam!")     == "nam");
	}

	CustomPostProc cpp_case(true, "", true);
	
	SUBCASE("check case correction") {
		CHECK(cpp_case.sanitizeWord("witajće") == "witajće");
		CHECK(cpp_case.sanitizeWord("K") == "k");
		CHECK(cpp_case.sanitizeWord("Nam,.-!") == "nam");
		
		CHECK(cpp_case.sanitizeWord("HRAJER") == "hrajer");
		CHECK(cpp_case.sanitizeWord("FELIKS") == "feliks");
		CHECK(cpp_case.sanitizeWord("RIČEL") == "ričel");
		
		CHECK(cpp_case.sanitizeWord("HRAJER!") == "hrajer");
		CHECK(cpp_case.sanitizeWord("FELIKS.") == "feliks");
		CHECK(cpp_case.sanitizeWord("RIČEL,") == "ričel");
	}
}

TEST_CASE("correction mode with list")
{
	CustomPostProc cpp(true, "replacement_list.txt");

	SUBCASE("check replacements without suffix") {
		CHECK(cpp.replaceWord("feliks") == "Feliks");
		CHECK(cpp.replaceWord("ričel")  == "Ričel");
		CHECK(cpp.replaceWord("ben")    == "Ben");
		CHECK(cpp.replaceWord("boese")  == "Boese");
	}
	
	SUBCASE("check no replacement if word does not match exactly") {
		CHECK(cpp.replaceWord("benej")   == "benej");
		CHECK(cpp.replaceWord("böseu")   == "böseu");
		CHECK(cpp.replaceWord("aben")    == "aben");
		CHECK(cpp.replaceWord("alričel") == "alričel");
	}

	SUBCASE("check different suffix lengths") {
		CHECK(cpp.replaceWord("chróšćic")   == "Chróšćic");
		CHECK(cpp.replaceWord("chróšćicy")  == "Chróšćicy");
		CHECK(cpp.replaceWord("chróšćicej") == "chróšćicej");
	}
	
	// probably unnecessary on word level
	SUBCASE("check space in replacee") {
		CHECK(cpp.replaceWord("sven erik")   == "Sven Erik");
	}
}

TEST_CASE("string length limit")
{
	CustomPostProc cpp(true, "", true, 30);
	
	SUBCASE("simple tests") {
		// no repetition
		CHECK(cpp.limitLine("abc", 1)                 == -1);
		CHECK(cpp.limitLine("bla abcabc", 1)          == -1);
		CHECK(cpp.limitLine("bla abcabcabc", 1)       == -1);
		CHECK(cpp.limitLine("bla abcabcabcabc", 1)    == -1);
		
		// repetition detected
		CHECK(cpp.limitLine("bla abcabcabcabcabcabcabcabcabc", 1) == 7);
		
		// repetition detected and word boundary found 
		CHECK(cpp.limitLine("bla hallihallo also hier wird es abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc abc", 1) == 36);
		
		// no repetition detected and word boundary not found
		CHECK(cpp.limitLine("bla abcabcabcabcabcabcabcabcabcblaabcabcabcabcabcabcabcabcabcaskldjghrasgsfdahfdsahgds", 1) == 60);
		
	}	
}
