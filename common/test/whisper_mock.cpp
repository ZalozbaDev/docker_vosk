#include "whisper_mock.h"

#include <stdlib.h>
#include <string.h>

// implementation-specific struct
struct whisper_context
{
	int id;
};

struct whisper_full_params default_params;

char resultText[1000];
int resultSegments = 0;

void whisper_free(struct whisper_context *ctx)
{
	free(ctx);
}

struct whisper_context *whisper_init_from_file_with_params(const char * path_model, struct whisper_context_params params)
{
	whisper_context *ctx = (whisper_context*) malloc(sizeof(struct whisper_context));
	return ctx;
}

int whisper_full(
	            struct whisper_context * ctx,
            struct whisper_full_params   params,
                           const float * samples,
                                   int   n_samples)
{
	return 0;	
}

int whisper_full_n_segments(struct whisper_context * ctx)
{
	return resultSegments;	
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

void whisper_mock_set_text(const char *text, int segments)
{
	resultText[0] = 0;
	strcat(resultText, text);
	resultSegments = segments;
}
