#ifndef WHISPER_IMPL_H
#define WHISPER_IMPL_H

#include "RecognitionResult.h"

#ifndef WHISPER_MOCK
#include "whisper.h"
#include "common.h" // ???
#else
#include "whisper_mock.h"
#endif

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

class WhisperImpl
{
public:
	WhisperImpl(std::string modelPath, std::string vosk_model_language, int whisper_max_context, bool whisper_no_timestamps, bool whisper_no_fallback, bool whisper_force_cpu);
	std::string getAnnouncementString(void);
	unsigned int getShortAudioBufferSizeSamples() { return pcm_buffer_short; }
	unsigned int getMaxAudioBufferSizeSamples()   { return pcm_buffer_max; }
	void run(std::vector<float>& pcmf32, std::vector<RecognizedToken>& tokens);
	~WhisperImpl();
private:
	// 1 second of audio is 16000 samples
	const unsigned int pcm_buffer_min   = WHISPER_SAMPLE_RATE * 1 + (WHISPER_SAMPLE_RATE / 100); // < 1 seconds will not work with whisper
	const unsigned int pcm_buffer_short = WHISPER_SAMPLE_RATE * 5; // < 5 seconds is short
	const unsigned int pcm_buffer_max   = WHISPER_SAMPLE_RATE * 29; // 29s, do not let audio grow past this value
    	
	// const int n_samples_30s  = (1e-3 * 30000.0) * WHISPER_SAMPLE_RATE;

	std::string m_modelPath;
	
	std::string m_vosk_model_language;
	int m_whisper_max_context;
	bool m_whisper_no_timestamps;
	bool m_whisper_no_fallback;
	bool m_whisper_force_cpu;
	
	struct whisper_context_params cparams;
	whisper_params default_params;
	struct whisper_context* ctx;	

	std::string getLocalTimeStamp();
};

#endif // WHISPER_IMPL_H
