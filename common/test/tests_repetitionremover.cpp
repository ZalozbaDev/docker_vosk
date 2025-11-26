
#include "doctest.h"

#include <RepetitionRemover.h>

#include <stddef.h>
#include <stdint.h>

#include <string>

#include <iostream>

TEST_CASE("simple tests")
{
	SUBCASE("check unmodified line") {
		Repetition rep;
		rep = RepetitionRemover::detectRepetitionByShift("abc");
		CHECK(rep.repetitions == 0);
	}

	SUBCASE("check one repetition") {
		Repetition rep;
		std::string testString = "bla abcabc";
		rep = RepetitionRemover::detectRepetitionByShift(testString);
		CHECK(rep.repetitions == 2);
		std::string reduced = RepetitionRemover::removeRepetitions(testString, rep);
		// std::cout << "|" << reduced << "|" << std::endl;
		CHECK(reduced == "bla abc");
	}

	SUBCASE("check two repetitions") {
		Repetition rep;
		std::string testString = "bla abcabcabc";
		rep = RepetitionRemover::detectRepetitionByShift(testString);
		CHECK(rep.repetitions == 3);
		std::string reduced = RepetitionRemover::removeRepetitions(testString, rep);
		// std::cout << "|" << reduced << "|" << std::endl;
		CHECK(reduced == "bla abc");
	}


}

TEST_CASE("real world tests")
{
	SUBCASE("check line without spaces") {
		Repetition rep;
		std::string testString = "třińdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźeńdźe";
		rep = RepetitionRemover::detectRepetitionByShift(testString);
		CHECK(rep.repetitions == 54);
		std::string reduced = RepetitionRemover::removeRepetitions(testString, rep);
		// std::cout << "|" << reduced << "|" << std::endl;
		CHECK(reduced == "třińdźe");
	}
	
	SUBCASE("check line with spaces and strange ending") {
		Repetition rep;
		std::string testString = "*spomniće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wěčeće, wě";
		rep = RepetitionRemover::detectRepetitionByShift(testString);
		CHECK(rep.repetitions == 30);
		std::string reduced = RepetitionRemover::removeRepetitions(testString, rep);
		// std::cout << "|" << reduced << "|" << std::endl;
		CHECK(reduced == "*spomniće, wěče");
	}
	
	SUBCASE("check line with spaces and strange ending") {
		Repetition rep;
		std::string testString = "lěće hodźinu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a wěruwajenu a w";
		rep = RepetitionRemover::detectRepetitionByShift(testString);
		CHECK(rep.repetitions == 26);
		std::string reduced = RepetitionRemover::removeRepetitions(testString, rep);
		// std::cout << "|" << reduced << "|" << std::endl;
		CHECK(reduced == "lěće hodźinu a wěruwaje");
	}

	SUBCASE("check line with spaces and strange ending") {
		Repetition rep;
		std::string testString = "a lubosći wokoło swětłaće a wěčnje wosomniwi a wěčnje wosomniwi a wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wěčnje wě";
		rep = RepetitionRemover::detectRepetitionByShift(testString);
		CHECK(rep.repetitions == 36);
		std::string reduced = RepetitionRemover::removeRepetitions(testString, rep);
		// std::cout << "|" << reduced << "|" << std::endl;
		CHECK(reduced == "a lubosći wokoło swětłaće a wěčnje wosomniwi a wěčnje wosomniwi a wěčnje");
	}

		
		/*
	
	
	"a lubosć ju poswjeća poswjeća poswjeća jow wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a wěmy a w"
	
	"zo móhli zhromadnje wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a wučerpjeć a w"
	
	"w krótkich lawdacijach hnydom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wědom hłósćicach, a wón wě"
	
	"a čest nam sy aktiwna eh našeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knjeza a jeho knje"
	
	"tež tu njeje njewidźu wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukowarjene wukow"
	*/
	
}


