
#include "RepetitionRemover.h"

#include <iostream>
#include <limits>

Repetition RepetitionRemover::detectRepetitionByShift(const std::string& s) {
    const size_t n = s.size();

    // Try all possible shifts (pattern lengths)
    for (size_t k = 1; k < n; ++k) {

        // Compare s[i] with s[i + k]
        size_t match_start = std::string::npos;
        size_t match_len = 0;

        for (size_t i = 0; i + k < n; ++i) {
            if (s[i] == s[i + k]) {
                if (match_start == std::string::npos)
                    match_start = i;
                match_len++;
            } else {
                match_start = std::string::npos;
                match_len = 0;
            }
        }
        
        if (match_len > 0)
        {
        	std::cout << "Testing potential match of len " << match_len << " with shift of " << k << std::endl;	
        }

        // Check if we got a meaningful repetition
        if (match_len >= k) {
            // repetition begins at the start of the match range
            size_t start = match_start;

            // repetition should run exactly to the end of the string (minus the shift applied)
            if (start + match_len == (n - k)) {
                size_t reps = match_len / k + 1; // +1 for the first implicit pattern

                return { start, k, reps };
            }
            else {
            	std::cout << "Disregarding match because it's length " << match_len 
            		<< " starting at " << start << " does not end at string length " 
            		<< n << "." << std::endl; 	
            }
        }
    }

    return {};
}

std::string RepetitionRemover::removeRepetitions(const std::string& s, const Repetition& r)
{
	// keep the first occurance of the repetition
	size_t end = r.start + r.length;
	
	std::string ret = s.substr(0, end);
	return ret;
}

/*
int main() {
    std::string input;
    std::cout << "Enter UTF-8 string: ";
    std::getline(std::cin, input);

    Repetition rep = detectRepetitionByShift(input);

    if (rep.start == std::string::npos) {
        std::cout << "No repetition found.\n";
    } else {
        std::cout << "Repetition detected:\n";
        std::cout << "  Start byte index: " << rep.start << "\n";
        std::cout << "  Pattern length:   " << rep.length << " bytes\n";
        std::cout << "  Repetitions:      " << rep.repetitions << "\n";

        std::cout << "  Pattern (UTF-8 bytes): \""
                  << input.substr(rep.start, rep.length)
                  << "\"\n";
    }

    return 0;
}
*/