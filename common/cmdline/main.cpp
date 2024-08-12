
#include <iostream>
#include <cstdint>


#include <sndfile.hh>

#include "VoskRecognizer.h"

#define BUFFER_LEN 65536

int main(int argc, char **argv)
{
	SndfileHandle file;
	int16_t buffer[BUFFER_LEN];
	
	if (argc < 1)
	{
		std::cout << "Error! Need to specify .wav file!" << std::endl;
		return 1;
	}
	
	file = SndfileHandle(argv[0]) ;

	std::cout << "File '" << argv[0] << "'." << std::endl;
	std::cout << "    Sample rate : " << file.samplerate() << std::endl;
	std::cout << "    Channels    : " << file.channels() << std::endl;
	std::cout << "    Size        : " << file.frames() << std::endl;

	file.read(buffer, BUFFER_LEN) ;

	
	return 0;
}
