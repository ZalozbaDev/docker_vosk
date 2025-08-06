#ifndef VOSK_RECOGNIZER_H
#define VOSK_RECOGNIZER_H

#include "RecognizerBase.h"

#include <iostream>
#include <vector>

#include <queue>

#include <thread>
#include <mutex>
#include <condition_variable>

extern "C" {
#include "vosk_api.h"
}

#include <VADWrapper.h>
#include <Resampler.h>
#include <RecognitionResult.h>
#include <AudioLogger.h>

#ifndef WHISPER_MOCK
#include "whisper.h"
#else
#include "whisper_mock.h"
#endif

#include <HunspellPostProc.h>
#include <CustomPostProc.h>

// command-line parameters from whisper.cpp/examples/main/main.cpp
struct whisper_params {
    int32_t n_threads     = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors  = 1;
    int32_t offset_t_ms   = 0;
    int32_t offset_n      = 0;
    int32_t duration_ms   = 0;
    int32_t progress_step = 5;
    int32_t max_context   = -1;
    int32_t max_len       = 0;
    int32_t best_of       = whisper_full_default_params(WHISPER_SAMPLING_GREEDY).greedy.best_of;
    int32_t beam_size     = whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH).beam_search.beam_size;
    int32_t audio_ctx     = 0;

    float word_thold      =  0.01f;
    float entropy_thold   =  2.40f;
    float logprob_thold   = -1.00f;
    float grammar_penalty = 100.0f;
    float temperature     = 0.0f;
    float temperature_inc = 0.2f;

    bool debug_mode      = false;
    bool translate       = false;
    bool detect_language = false;
    bool diarize         = false;
    bool tinydiarize     = false;
    bool split_on_word   = false;
    bool no_fallback     = false;
    bool output_txt      = false;
    bool output_vtt      = false;
    bool output_srt      = false;
    bool output_wts      = false;
    bool output_csv      = false;
    bool output_jsn      = false;
    bool output_jsn_full = false;
    bool output_lrc      = false;
    bool no_prints       = false;
    bool print_special   = false;
    bool print_colors    = false;
    bool print_progress  = false;
    bool no_timestamps   = false;
    bool log_score       = false;
    bool use_gpu         = true;
    bool flash_attn      = false;

    std::string language  = "en";
    std::string prompt;
    std::string font_path = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";
    std::string model     = "models/ggml-base.en.bin";
    std::string grammar;
    std::string grammar_rule;

    // [TDRZ] speaker turn string
    std::string tdrz_speaker_turn = " [SPEAKER_TURN]"; // TODO: set from command line

    // A regular expression that matches tokens to suppress
    std::string suppress_regex;

    std::string openvino_encode_device = "CPU";

    std::string dtw = "";

    std::vector<std::string> fname_inp = {};
    std::vector<std::string> fname_out = {};

    // grammar_parser::parse_state grammar_parsed;
};

//////////////////////////////////////////////
class VoskRecognizer:public RecognizerBase
{
public:
	VoskRecognizer(int modelId, float sample_rate, const char *configPath, int aggressiveness=2);
	virtual ~VoskRecognizer(void);
	
	virtual int getInstanceId(void)                               override { return m_instanceId; }
	virtual int getModelInstanceId(void)                          override { return m_modelInstanceId; }
	virtual float getSampleRate(void)                             override { return m_inputSampleRate; }
	virtual void setDetailedResult(bool detailsOn)                override;
	virtual int acceptWaveform(const char *data, int length)      override;
	virtual bool getRecognizerBusy(bool audioQueueOnly = false)   override;
	virtual const char* getPartialResult(void)                    override;
	virtual const char* getFinalResult(void)                      override;
	virtual bool getPartialStatus(void)                           override;
	virtual std::unique_ptr<FinalResult> getFinalResultData(void) override;
	virtual int getFrameResolution(void)                          override;

	// TBD move to base?
	void setTimeStamp(int64_t seconds, int64_t uSeconds);
	void resultCallback(char* word, unsigned int startTimeMs, unsigned int endTimeMs, float negLogLikelihood);
	
private:
	static const ssize_t m_processingSampleRate = 16000;
	
	static const int m_numberModelAnnouncements = 3;
	
	static int voskRecognizerInstanceId;

	int m_instanceId;
	int m_modelInstanceId;
	float m_inputSampleRate;
	bool m_libraryLoaded;
	VoskRecognizerState m_recoState;
	uint64_t m_vadFrameCounter;
	
	std::thread *recoWorkerThread;
	bool threadRunning;
	std::deque<std::unique_ptr<AudioPacket>> audioPackets;
	std::mutex audioPacketMutex;
	std::condition_variable audioPacketNotify;
	void workerThreadFunc(void);

	std::string m_configPath;

	whisper_params default_params;
	const int n_samples_30s  = (1e-3 * 30000.0) * WHISPER_SAMPLE_RATE;
    
	std::vector<float> pcmf32;
	
	// 1 second of audio is 16000 samples
	static const unsigned int pcm_buffer_min   = WHISPER_SAMPLE_RATE * 1 + (WHISPER_SAMPLE_RATE / 100); // < 1 seconds will not work with whisper
	static const unsigned int pcm_buffer_short = WHISPER_SAMPLE_RATE * 5; // < 5 seconds is short
	static const unsigned int pcm_buffer_max   = WHISPER_SAMPLE_RATE * 29; // 29s, do not let audio grow past this value
	
	VADWrapper *vad;
	Resampler  *resample;
	
	char* leftOverData;
	int leftOverDataLen = 0;
	
	std::chrono::time_point<std::chrono::system_clock> clientTimeStamp;
	
	std::vector<std::unique_ptr<RecognitionResult>> partialResult;
	std::mutex partialResultMutex;
	
	std::deque<std::unique_ptr<FinalResult>>        finalResults;
	std::mutex finalResultMutex;
	
	// to avoid early deletion of string objects, use preallocated memory for the most recent string
	char partialResultBuffer[1000];
	char finalResultBuffer[1000];
	bool detailedResults;
	
	void promoteToFinalResult(void);
	void runWhisper(struct whisper_context* ctx);
	
	AudioLogger *audioLogger;
	
	HunspellPostProc *hpp;
	CustomPostProc *cpp;
	
	// additional options from envvars
	std::string env_vosk_model_language;
	int         env_whisper_max_context;
	bool        env_whisper_no_timestamps;
};

#endif // VOSK_RECOGNIZER_H

