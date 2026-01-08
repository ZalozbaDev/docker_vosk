#ifndef WHISPER_MOCK_H_
#define WHISPER_MOCK_H_

#include <stddef.h>
#include <stdint.h>

#define WHISPER_SAMPLE_RATE 16000

typedef int32_t whisper_token;

enum whisper_alignment_heads_preset {
	WHISPER_AHEADS_NONE,
	WHISPER_AHEADS_N_TOP_MOST,  // All heads from the N-top-most text-layers
	WHISPER_AHEADS_CUSTOM,
	WHISPER_AHEADS_TINY_EN,
	WHISPER_AHEADS_TINY,
	WHISPER_AHEADS_BASE_EN,
	WHISPER_AHEADS_BASE,
	WHISPER_AHEADS_SMALL_EN,
	WHISPER_AHEADS_SMALL,
	WHISPER_AHEADS_MEDIUM_EN,
	WHISPER_AHEADS_MEDIUM,
	WHISPER_AHEADS_LARGE_V1,
	WHISPER_AHEADS_LARGE_V2,
	WHISPER_AHEADS_LARGE_V3,
	WHISPER_AHEADS_LARGE_V3_TURBO,
};

typedef struct whisper_ahead {
	int n_text_layer;
	int n_head;
} whisper_ahead;

typedef struct whisper_aheads {
	size_t n_heads;
	const whisper_ahead * heads;
} whisper_aheads;

struct whisper_context_params {
	bool  use_gpu;
	bool  flash_attn;
	int   gpu_device;  // CUDA device

	// [EXPERIMENTAL] Token-level timestamps with DTW
	bool dtw_token_timestamps;
	enum whisper_alignment_heads_preset dtw_aheads_preset;

	int dtw_n_top;
	struct whisper_aheads dtw_aheads;

	size_t dtw_mem_size; // TODO: remove
};

enum whisper_sampling_strategy {
    WHISPER_SAMPLING_GREEDY,      // similar to OpenAI's GreedyDecoder
    WHISPER_SAMPLING_BEAM_SEARCH, // similar to OpenAI's BeamSearchDecoder
};

struct whisper_full_params {
        enum whisper_sampling_strategy strategy;
        
        // tokens to provide to the whisper decoder as initial prompt
        // these are prepended to any existing text context from a previous call
        const char * initial_prompt;
        const whisper_token * prompt_tokens;
        int prompt_n_tokens;

        float temperature;      // initial decoding temperature, ref: https://ai.stackexchange.com/a/32478
        float max_initial_ts;   // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L97
        float length_penalty;   // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L267

        // fallback parameters
        // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
        float temperature_inc;
        float entropy_thold;    // similar to OpenAI's "compression_ratio_threshold"
        float logprob_thold;
        float no_speech_thold;  // TODO: not implemented

         // for auto-detection, set to nullptr, "" or "auto"
        const char * language;
        bool detect_language;

        // [EXPERIMENTAL] token-level timestamps
        bool  token_timestamps; // enable token-level timestamps
        float thold_pt;         // timestamp token probability threshold (~0.01)
        float thold_ptsum;      // timestamp token sum probability threshold (~0.01)
        int   max_len;          // max segment length in characters
        bool  split_on_word;    // split on word rather than on token (when used with max_len)
        int   max_tokens;       // max tokens per segment (0 = no limit)

        // [EXPERIMENTAL] speed-up techniques
        // note: these can significantly reduce the quality of the output
        bool speed_up;          // speed-up the audio by 2x using Phase Vocoder
        bool debug_mode;        // enable debug_mode provides extra info (eg. Dump log_mel)
        int  audio_ctx;         // overwrite the audio context size (0 = use default)

        // [EXPERIMENTAL] [TDRZ] tinydiarize
        bool tdrz_enable;       // enable tinydiarize speaker turn detection

        // A regular expression that matches tokens to suppress
        const char * suppress_regex;

	    int n_threads;
        int n_max_text_ctx;     // max tokens to use from past text as prompt for the decoder
        int offset_ms;          // start offset in ms
        int duration_ms;        // audio duration to process in ms

	    bool translate;
        bool no_context;        // do not use past transcription (if any) as initial prompt for the decoder
        bool no_timestamps;     // do not generate timestamps
        bool single_segment;    // force single segment output (useful for streaming)
        bool print_special;     // print special tokens (e.g. <SOT>, <EOT>, <BEG>, etc.)
        bool print_progress;    // print progress information
        bool print_realtime;    // print results from within whisper.cpp (avoid it, use callback instead)
        bool print_timestamps;  // print timestamps for each text segment when printing realtime
        
        struct {
            int best_of;    // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264
        } greedy;

        struct {
            int beam_size;  // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265

            float patience; // TODO: not implemented, ref: https://arxiv.org/pdf/2204.05424.pdf
        } beam_search;

};

void whisper_free      (struct whisper_context * ctx);

struct whisper_context * whisper_init_from_file_with_params  (const char * path_model,              struct whisper_context_params params);

int whisper_full(
                struct whisper_context * ctx,
            struct whisper_full_params   params,
                           const float * samples,
                                   int   n_samples);

int whisper_full_parallel(
                struct whisper_context * ctx,
            struct whisper_full_params   params,
                           const float * samples,
                                   int   n_samples,
                                   int   n_processors);

int whisper_full_n_tokens (struct whisper_context * ctx, int segment);

const char * whisper_full_get_token_text           (struct whisper_context * ctx, int i_segment, int j);

float whisper_full_get_token_p           (struct whisper_context * ctx, int i_segment, int j);

int whisper_full_n_segments           (struct whisper_context * ctx);

int64_t whisper_full_get_segment_t0           (struct whisper_context * ctx, int i_segment);

int64_t whisper_full_get_segment_t1           (struct whisper_context * ctx, int i_segment);

const char * whisper_full_get_segment_text           (struct whisper_context * ctx, int i_segment);

struct whisper_full_params whisper_full_default_params(enum whisper_sampling_strategy strategy);

struct whisper_context_params whisper_context_default_params();

void whisper_mock_set_text(const char *text, int segments);

void whisper_mock_set_overload(bool enableAllocOverload, bool enableExecOverload);

#endif  // WHISPER_MOCK_H_
