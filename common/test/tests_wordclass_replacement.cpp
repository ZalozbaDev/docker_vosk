
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

TEST_CASE("test date parser")
{
	WordClassPostProc wcpp(true);
	
	SUBCASE("check negative date without delimiter") {
		CHECK(wcpp.processLine("dnja <DATE>+1-61+0</DATE> zemrě") == "dnja 01.11. zemrě");
	}

	SUBCASE("check negative dates with delimiter") {
		CHECK(wcpp.processLine("mjez <DATE>+1-61+0<->+10-61+0</DATE> njejsu") == "mjez 01.11.-10.11. njejsu");
	}

	SUBCASE("check positive date without delimiter") {
		CHECK(wcpp.processLine("dnja <DATE>+1+0+0</DATE> zemrě") == "dnja 01.01. zemrě");
	}

	SUBCASE("check negative dates with delimiter") {
		CHECK(wcpp.processLine("mjez <DATE>+1+31+0<->+10+31+0</DATE> njejsu") == "mjez 01.02.-10.02. njejsu");
	}

	SUBCASE("check date error without delimiter") {
		CHECK(wcpp.processLine("dnja <DATE>+1sfdhsteh+0+0</DATE> zemrě") == "dnja ??? zemrě");
	}

	SUBCASE("check date error with delimiter first part") {
		CHECK(wcpp.processLine("mjez <DATE>+1-61shfdjghjgd+0<->+10-61+0</DATE> njejsu") == "mjez ???-10.11. njejsu");
	}

	SUBCASE("check date error with delimiter second part") {
		CHECK(wcpp.processLine("mjez <DATE>+1-61+0<->+10-61$%&+0</DATE> njejsu") == "mjez 01.11.-??? njejsu");
	}

}

TEST_CASE("test time parser")
{
	WordClassPostProc wcpp(true);
	
	SUBCASE("check simple time") {
		CHECK(wcpp.processLine("w <TIME>+840+0+0</TIME> hodźin") == "w 14:00 hodźin");
	}

	SUBCASE("check time with minutes") {
		CHECK(wcpp.processLine("w <TIME>+840+15+0</TIME> hodźin") == "w 14:15 hodźin");
	}

	SUBCASE("check time range") {
		CHECK(wcpp.processLine("w <TIME>+720+5+40<->+780+30</TIME> hodźin") == "w 12:45-13:30 hodźin");
	}

	SUBCASE("check eval error first time") {
		CHECK(wcpp.processLine("w <TIME>+720+sfdhsg5+40<->+780+30</TIME> hodźin") == "w ???-13:30 hodźin");
	}

	SUBCASE("check eval error second time") {
		CHECK(wcpp.processLine("w <TIME>+720+5+40<->+78sdghsfdh0+30</TIME> hodźin") == "w 12:45-??? hodźin");
	}

}

TEST_CASE("test weekday parser")
{
	WordClassPostProc wcpp(true);
	
	SUBCASE("check first weekday") {
		CHECK(wcpp.processLine("w <WEEKDAY>+1</WEEKDAY> zetkamy so") == "w pón. zetkamy so");
	}

	SUBCASE("check last weekday") {
		CHECK(wcpp.processLine("w <WEEKDAY>+7</WEEKDAY> zetkamy so") == "w nje. zetkamy so");
	}

	SUBCASE("check invalid weekday") {
		CHECK(wcpp.processLine("w <WEEKDAY>+0</WEEKDAY> zetkamy so") == "w ??? zetkamy so");
	}

	SUBCASE("check invalid weekday") {
		CHECK(wcpp.processLine("w <WEEKDAY>+8</WEEKDAY> zetkamy so") == "w ??? zetkamy so");
	}

}

TEST_CASE("test other numbers (ORDINAL/CARDINAL)")
{
	WordClassPostProc wcpp(true);
	
	SUBCASE("check simple number") {
		CHECK(wcpp.processLine("čisło <CARDINAL>+200+9+50</CARDINAL>") == "čisło 259");
	}

	SUBCASE("check bigger number") {
		CHECK(wcpp.processLine("čisło <CARDINAL>(+3+20)*1000+400+7+30</CARDINAL>") == "čisło 23437");
	}

	SUBCASE("check simple ordinal") {
		CHECK(wcpp.processLine("na <ORDINAL>+20+3</ORDINAL> městnje") == "na 23. městnje");
	}

}

TEST_CASE("test other strings (NAME/SNAME/GPE)")
{
	WordClassPostProc wcpp(true);
	
	SUBCASE("check simple name") {
		CHECK(wcpp.processLine("kapłan <NAME>markus</NAME> je") == "kapłan markus je");
	}

	SUBCASE("check simple sname") {
		CHECK(wcpp.processLine("zemrětoho <NAME>jakuba</NAME> <SNAME>zarjenka</SNAME> a") == "zemrětoho jakuba zarjenka a");
	}

	SUBCASE("check simple location") {
		CHECK(wcpp.processLine("z <GPE>njedźichow</GPE> na") == "z njedźichow na");
	}

}

TEST_CASE("test corrupted input strings")
{
	WordClassPostProc wcpp(true);

	SUBCASE("check wrong closing tag") {
		CHECK(wcpp.processLine("zemrětoho <NAME>jakuba</NAME> <SNAME>zarjenka</NAME> a") == "zemrětoho jakuba <SNAME>zarjenka</NAME> a");
	}

	SUBCASE("check another wrong closing tag") {
		CHECK(wcpp.processLine("z <GPE>njedźichow</NAME> na") == "z <GPE>njedźichow</NAME> na");
	}

	
}
