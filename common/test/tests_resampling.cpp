
#include "doctest.h"

#include <ResamplerWebRTC_8_16.h>

#include <stddef.h>
#include <stdint.h>


#include <iostream>

#include <fstream>
#include <vector>

std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Failed to open file");

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);

    if (!file.read(buffer.data(), size))
        throw std::runtime_error("Failed to read file");

    std::cout << "Infile size (bytes) == " << size << "." << std::endl;
    
    return buffer;
}

void writeBinaryFile(const std::string& filename,
                     const std::vector<int16_t>& data)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file for writing");

    std::cout << "Outfile size (bytes) == " << (data.size() * sizeof(int16_t)) << "." << std::endl;
    
    file.write(
        reinterpret_cast<const char*>(data.data()),
        data.size() * sizeof(int16_t)
    );

    if (!file)
        throw std::runtime_error("Failed to write data");
}

TEST_CASE("test uLaw resampling")
{
	SUBCASE("test resampling of provided file") {
		
		Resampler* resamplePhone = new ResamplerWebRTC_8_16();
		
		// TBD ouch!!!!
		std::vector<char> inData = readFile("incoming.mulaw");
		
		int16_t target[160];
		
		std::vector<int16_t> out;
		
		for (int i = 0; i < 76640; i+= 80)
		{
			resamplePhone->resample((int16_t*) &inData[i], &target[0], 80);
			
			out.insert(out.end(), std::begin(target), std::end(target));
		}
		
		writeBinaryFile("outgoingpcm16khz.raw", out);
		
		delete resamplePhone;
		
	}
	
	

}
