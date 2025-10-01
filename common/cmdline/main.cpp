
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

static std::string to_timestamp(int frameResolutionMs, uint64_t t) {
 
    // one VAD frame is how many ms? 
    uint64_t msec = t * frameResolutionMs;
     
     
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

static int subtitle_index = 1;

static void process_subtitle(std::unique_ptr<FinalResult> res, std::ofstream& transcript, std::ofstream& subtitles, float confidenceThreshold, int frameResolutionMs)
{
	// do not process empty text
	if (res->text.length() > 0)
	{
		// do not process text below confidence threshold
		if (res->confidence > confidenceThreshold)
		{
			uint64_t frameCounterDiff = res->frameCounterEnd - res->frameCounterStart;
			float frameLenMs = frameResolutionMs * frameCounterDiff;
			
			// do not process short frames if confidence filter is on (threshold > 0) 
			if ((confidenceThreshold < 0) || (frameLenMs > 1000.0f))
			{
				transcript << res->text << std::endl;
				
				subtitles << subtitle_index << std::endl;
				subtitles << to_timestamp(frameResolutionMs, res->frameCounterStart) << " --> " << to_timestamp(frameResolutionMs, res->frameCounterEnd) << std::endl;
				subtitles << res->text << std::endl;
				subtitles << std::endl;
				subtitle_index++;
				
				std::cout << res->text << std::endl;
			}
			else
			{
				std::cout << "#### Skipping short line " << frameLenMs << "ms: " << res->text << std::endl;
			}
		}
		else
		{
			std::cout << "#### Skipping line with bad confidence " << res->confidence << ": " << res->text << std::endl;
		}
	}
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
	int vad_aggressiveness = 2;
	int frameResolutionMs;
	float confidenceThreshold = -10000.0f;
	
	if (argc < 4)
	{
		std::cout << "Error! Need to specify at least model path, .wav file and destination path! [vad_aggressiveness] [whisper_lang] [whisper_max_ctx] [whisper_no_timestamps] [WebRTC|Silero] [confidenceThreshold]" << std::endl;
		std::cout << "Example: ./main ./model/data/merged_47_nnet_v3.cfg ./testdata/0001_citanje.wav testresults/" << std::endl;
		std::cout << "Example: ./main ./model/data/merged_47_nnet_v3.cfg ./testdata/0001_citanje.wav testresults/ 2" << std::endl;
		std::cout << "Example: ./main ./ggml/ggml-model_v3.bin ./testdata/0001_citanje.wav testresults/ 2 czech 0 true" << std::endl;
		std::cout << "Example: ./main ./ggml/ggml-model_v3.bin ./testdata/0001_citanje.wav testresults/ 2 czech 0 true Silero" << std::endl;
		std::cout << "Example: ./main ./ggml/ggml-model_v3.bin ./testdata/0001_citanje.wav testresults/ 2 czech 0 true Silero 0.9" << std::endl;
		return 1;
	}
	
	if (argc >= 5)
	{
		vad_aggressiveness = std::stoi(argv[4]);
		if ((vad_aggressiveness < 1) || (vad_aggressiveness > 3))
		{
			std::cout << "VAD aggressiveness " << vad_aggressiveness << " out of range 1..3!" << std::endl;
			return 1;
		}
		std::cout << "Setting custom VAD aggressiveness=" << vad_aggressiveness << "." << std::endl;
	}
	
	// handle additional envvars
	
	setenv("VOSK_SUBWORD_REGEX", "# #", 1); // API not part of C++
	setenv("VOSK_WHISPER_USE_CPU", "false", 1);
	
	if (argc >= 6)
	{
		setenv("VOSK_MODEL_LANGUAGE", argv[5], 1);
	}
	if (argc >= 7)
	{
		setenv("VOSK_WHISPER_MAX_CONTEXT", argv[6], 1);
	}
	if (argc >= 8)
	{
		setenv("VOSK_WHISPER_DISABLE_TIMESTAMPS", argv[7], 1);
	}
	if (argc >= 9)
	{
		setenv("VOSK_VAD_ALGO", argv[8], 1);
	}
	if (argc >= 10)
	{
		confidenceThreshold = std::stof(std::string(argv[9]));
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
	
	VoskRecognizer v(1, 48000, argv[1], vad_aggressiveness);
	
	v.setDetailedResult(true);
	
	frameResolutionMs = v.getFrameResolution();
	
	std::ofstream transcript;
	transcript.open(std::string(argv[3]) + "/transcript.txt", std::ios::out | std::ios::trunc);
	
	std::ofstream subtitles;
	subtitles.open(std::string(argv[3]) + "/subtitles.srt", std::ios::out | std::ios::trunc);
	
	index = 0;
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
			process_subtitle(std::move(res), transcript, subtitles, confidenceThreshold, frameResolutionMs);
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
		if (v.getRecognizerBusy(true) == true)
		{
			std::cout << "Throttle!" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	
	while (v.getRecognizerBusy(false) == true)
	{
		std::unique_ptr<FinalResult> res = v.getFinalResultData();
		process_subtitle(std::move(res), transcript, subtitles, confidenceThreshold, frameResolutionMs);
		
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	
	transcript.flush();
	transcript.close();
	
	subtitles.flush();
	subtitles.close();
	
	return 0;
}
