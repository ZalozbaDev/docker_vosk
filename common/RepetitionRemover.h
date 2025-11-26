#ifndef REPETITION_REMOVER_H
#define REPETITION_REMOVER_H

#include <stddef.h>

#include <string>

struct Repetition {
    size_t start = std::string::npos;
    size_t length = 0;      // pattern length in bytes
    size_t repetitions = 0; // how many times it repeats at the end
};

class RepetitionRemover {
public:
	static Repetition detectRepetitionByShift(const std::string& s);
	static std::string removeRepetitions(const std::string& s, const Repetition& r);
};

#endif // REPETITION_REMOVER_H
