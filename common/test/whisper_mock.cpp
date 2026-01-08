#include "whisper_mock.h"

#include <stdlib.h>
#include <string.h>
#include <iostream>

// implementation-specific struct
struct whisper_context
{
	int id;
};

struct whisper_full_params default_params;

char resultText[1000];
int resultSegments = 0;

bool allocOverload = false;
bool execOverload = false;

void whisper_free(struct whisper_context *ctx)
{
	free(ctx);
}

struct whisper_context *whisper_init_from_file_with_params(const char * path_model, struct whisper_context_params params)
{
	if (allocOverload == false)
	{
		std::cout << "<<<< Whisper mock alloc ok!" << std::endl;
		whisper_context *ctx = (whisper_context*) malloc(sizeof(struct whisper_context));
		return ctx;
	}
	else
	{
		std::cout << ">>>> Whisper mock alloc overload!" << std::endl;
		return nullptr;	
	}
}

int whisper_full(
	            struct whisper_context * ctx,
            struct whisper_full_params   params,
                           const float * samples,
                                   int   n_samples)
{
	if (execOverload == false)
	{
		std::cout << "<<<< Whisper mock exec overload!" << std::endl;
		return 0;
	}
	else
	{
		std::cout << ">>>> Whisper mock exec overload!" << std::endl;
		return -1;
	}
}

int whisper_full_parallel(
                struct whisper_context * ctx,
            struct whisper_full_params   params,
                           const float * samples,
                                   int   n_samples,
                                   int   n_processors)
{
	return whisper_full(ctx, params, samples, n_samples);
}

int whisper_full_n_tokens (struct whisper_context * ctx, int segment)
{
	return 5;
}

int whisper_full_n_segments(struct whisper_context * ctx)
{
	return resultSegments;	
}

const char * whisper_full_get_token_text(struct whisper_context * ctx, int i_segment, int j)
{
	return resultText;
}

float whisper_full_get_token_p           (struct whisper_context * ctx, int i_segment, int j)
{
	return 0.93f;	
}

int64_t whisper_full_get_segment_t0(struct whisper_context * ctx, int i_segment)
{
	return 0;	
}

int64_t whisper_full_get_segment_t1(struct whisper_context * ctx, int i_segment)
{
	return 1;	
}

const char * whisper_full_get_segment_text(struct whisper_context * ctx, int i_segment)
{
	return resultText;	
}

struct whisper_full_params whisper_full_default_params(enum whisper_sampling_strategy strategy)
{
	return default_params;
}

struct whisper_context_params whisper_context_default_params() {
    struct whisper_context_params result = {
        /*.use_gpu              =*/ true,
        /*.flash_attn           =*/ false,
        /*.gpu_device           =*/ 0,

        /*.dtw_token_timestamps =*/ false,
        /*.dtw_aheads_preset    =*/ WHISPER_AHEADS_NONE,
        /*.dtw_n_top            =*/ -1,
        /*.dtw_aheads           =*/ {
            /*.n_heads          =*/ 0,
            /*.heads            =*/ NULL,
        },
        /*.dtw_mem_size         =*/ 1024*1024*128,
    };
    return result;
}

void whisper_mock_set_text(const char *text, int segments)
{
	resultText[0] = 0;
	strcat(resultText, text);
	resultSegments = segments;
}

void whisper_mock_set_overload(bool enableAllocOverload, bool enableExecOverload)
{
	allocOverload = enableAllocOverload;	
	execOverload = enableExecOverload;	
}
