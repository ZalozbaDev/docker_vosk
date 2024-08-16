
#include <iostream>
#include <fstream>

#include <iomanip>

#include <cstdint>
#include <cstdlib>

#include <chrono>

#include <sndfile.hh>

#include "VoskRecognizer.h"

#define BUFFER_LEN 65536

#define LOG_BUFFER_LEN 32768

static std::string to_timestamp(uint64_t t) {

	// one VAD frame == 10ms
    uint64_t msec = t * 10;
    
    // compute hour and remember fraction of ms
    uint64_t hr = msec / (1000 * 60 * 60);
    msec = msec - hr * (1000 * 60 * 60);
    
    // compute minutes and remember fraction of ms
    uint64_t min = msec / (1000 * 60);
    msec = msec - min * (1000 * 60);
    
    // compute seconds and remember fraction of ms
    uint64_t sec = msec / 1000;
    msec = msec - sec * 1000;

    std::cout << t << " --> " << hr << ":" << min << ":" << sec << "," << msec << std::endl;
    
    char buf[32];
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u%s%03u", (int) hr, (int) min, (int) sec, ",", (int) msec);

    return std::string(buf);
}

int main(int argc, char **argv)
{
	SndfileHandle file;
	short buffer[BUFFER_LEN];
	// char logBuffer[LOG_BUFFER_LEN];
	int channels;
	int samplerate;
	int format;
	sf_count_t size;
	sf_count_t index;
	// int logsize;
	int subtitle_index;
	
	if (argc < 3)
	{
		std::cout << "Error! Need to specify model path and .wav file!" << std::endl;
		return 1;
	}
	
	file = SndfileHandle(argv[2]) ;

	// switch on scaling for reading float files
	file.command(SFC_SET_SCALE_FLOAT_INT_READ, NULL, SF_TRUE);
	
	/*
	logsize = file.command(SFC_GET_LOG_INFO, logBuffer, sizeof(logBuffer));
	if (logsize > 0)
	{
		logBuffer[logsize] = 0;
		std::cout << "LOG: " << std::string(logBuffer) << std::endl;
	}
	*/
	
	samplerate = file.samplerate();
	channels = file.channels();
	size = file.frames();
	format = file.format();
	
	// reduce float digits
	std::setprecision(2);
	
	
	std::cout << "File '" << argv[2] << "'." << std::endl;
	std::cout << "    Sample rate : " << samplerate << std::endl;
	std::cout << "    Channels    : " << channels << std::endl;
	std::cout << "    Size        : " << size << std::endl;
	std::cout << "    Format      : " << format << std::endl;

	if ((channels != 1) || (samplerate != 48000))
	{
		std::cout << "Error in .wav file format!" << std::endl;	
		return 1;
	}
	
	// not part of C++
	setenv("VOSK_SUBWORD_REGEX", "# #", 1);
	
	VoskRecognizer v(1, 48000, argv[1]);
	
	v.setDetailedResult(true);
	
	std::ofstream transcript;
	transcript.open("transcript.txt", std::ios::out | std::ios::trunc);
	
	std::ofstream subtitles;
	subtitles.open("subtitles.srt", std::ios::out | std::ios::trunc);
	
	index = 0;
	subtitle_index = 1;
	while (index < size)
	{
		int res;
		
		sf_count_t amount = file.read(buffer, BUFFER_LEN);
		
		// std::cout << "Error: " << file.error() << std::endl;
		
		/*
		std::ios_base::fmtflags f(std::cout.flags());  // save flags state
		for (int k = 0; k < BUFFER_LEN; k ++)
		{
			std::cout << std::hex << buffer[k] << " ";
		}
		std::cout << std::endl;
		std::cout.flags(f);  // restore flags state
		*/
		
		index += amount;
		
		res = v.acceptWaveform((const char *) buffer, amount * 2);
		if (res == 0)
		{
			std::cout << v.getPartialStatus() << std::endl;	
		}
		else
		{
			std::unique_ptr<FinalResult> res = v.getFinalResultData();
			
			transcript << res->text << std::endl;
			
			subtitles << subtitle_index << std::endl;
			subtitles << to_timestamp(res->frameCounterStart) << " --> " << to_timestamp(res->frameCounterEnd) << std::endl;
			subtitles << res->text << std::endl;
			subtitles << std::endl;
			subtitle_index++;
			
			std::cout << res->text << std::endl;	
		}

		/*
		logsize = file.command(SFC_GET_LOG_INFO, logBuffer, sizeof(logBuffer));
		if (logsize > 0)
		{
			logBuffer[logsize] = 0;
			std::cout << "LOG: " << std::string(logBuffer) << std::endl;
		}
		*/
	
		std::cout << "Read " << amount << " samples, total=" << index << ", sec=" << (index / 48000) << ", " << (((float) index / (float) size) * 100.0f) << "%." << std::endl;
		
		// for a threaded impl we are providing too much data at once, so slow it down here 
		if (v.getWaveformBufferPackets() > 2)
		{
			std::cout << "Throttle!" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	
	while (v.getWaveformBufferPackets() > 0)
	{
		std::unique_ptr<FinalResult> res = v.getFinalResultData();
		
		if (res->text.length() > 0)
		{
			transcript << res->text << std::endl;
			
			subtitles << subtitle_index << std::endl;
			subtitles << to_timestamp(res->frameCounterStart) << " --> " << to_timestamp(res->frameCounterEnd) << std::endl;
			subtitles << res->text << std::endl;
			subtitles << std::endl;
			subtitle_index++;
			
			std::cout << res->text << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	
	transcript.flush();
	transcript.close();
	
	subtitles.flush();
	subtitles.close();
	
	return 0;
}
