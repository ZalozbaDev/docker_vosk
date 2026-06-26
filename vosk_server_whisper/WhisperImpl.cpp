#include "WhisperImpl.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

//////////////////////////////////////////////
WhisperImpl::WhisperImpl(std::string modelPath, std::string vosk_model_language, int whisper_max_context, bool whisper_no_timestamps, bool whisper_no_fallback, bool whisper_force_cpu)
{
	m_modelPath = modelPath;
	
	m_vosk_model_language   = vosk_model_language;
	m_whisper_max_context   = whisper_max_context;
	m_whisper_no_timestamps = whisper_no_timestamps;
	m_whisper_no_fallback   = whisper_no_fallback;
	m_whisper_force_cpu     = whisper_force_cpu;
	
	// whisper init
	cparams = whisper_context_default_params();
	
	cparams.use_gpu = !m_whisper_force_cpu;
	cparams.flash_attn = true;
	cparams.dtw_token_timestamps = false;
	
	ctx = whisper_init_from_file_with_params(m_modelPath.c_str(), cparams);	
}

//////////////////////////////////////////////
std::string WhisperImpl::getAnnouncementString(void)
{
	// older versions required fixed version string
	// std::string whisperStr = "whisper.cpp 1.7.4";
	
	// use whisper version string once available via API
	std::string whisperStr = "whisper.cpp " + std::string(whisper_version());
	
	std::string modelStr = std::regex_replace(m_modelPath, std::regex("(\\/|\\.)"), "-");
	
	return whisperStr + " : " + modelStr;
}

// #define MEASURE_WHISPER_TIME

//////////////////////////////////////////////
void WhisperImpl::run(std::vector<float>& pcmf32, std::vector<RecognizedToken>& tokens)
{
	// run whisper on the current state of audio buffer
	whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.strategy = (default_params.beam_size > 1) ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY;
	
    wparams.print_realtime   = false;
	wparams.print_progress   = false;
	wparams.print_timestamps = m_whisper_no_timestamps; // !default_params.no_timestamps;
	wparams.print_special    = default_params.print_special;
	wparams.translate        = default_params.translate;
	if (m_vosk_model_language == "auto")
	{
		// whisper.cpp's default is "en", so if we really want "auto", we must say so explicitly
		// wparams.language         = default_params.language.c_str();
		wparams.language         = "auto";
	}
	else
	{
		wparams.language         = m_vosk_model_language.c_str();
	}
    wparams.detect_language  = default_params.detect_language;
    wparams.n_threads        = default_params.n_threads;
    wparams.n_max_text_ctx   = default_params.max_context >= 0 ? default_params.max_context : wparams.n_max_text_ctx;
	if (m_whisper_max_context != -1)
	{
		wparams.n_max_text_ctx = m_whisper_max_context;
	}
    wparams.offset_ms        = default_params.offset_t_ms;
    wparams.duration_ms      = default_params.duration_ms;

    wparams.token_timestamps = default_params.output_wts || default_params.output_jsn_full || default_params.max_len > 0;
    wparams.thold_pt         = default_params.word_thold;
    wparams.max_len          = default_params.output_wts && default_params.max_len == 0 ? 60 : default_params.max_len;
    wparams.split_on_word    = default_params.split_on_word;
    wparams.audio_ctx        = default_params.audio_ctx;

    wparams.debug_mode       = default_params.debug_mode;

    wparams.tdrz_enable      = default_params.tinydiarize; // [TDRZ]

    wparams.suppress_regex   = default_params.suppress_regex.empty() ? nullptr : default_params.suppress_regex.c_str();

    wparams.initial_prompt       = default_params.prompt.c_str();
    wparams.carry_initial_prompt = default_params.carry_initial_prompt;

    wparams.greedy.best_of        = default_params.best_of;
    wparams.beam_search.beam_size = default_params.beam_size;

    wparams.temperature_inc  = m_whisper_no_fallback ? 0.0f : default_params.temperature_inc;
    wparams.temperature      = default_params.temperature;
    wparams.no_speech_thold  = default_params.no_speech_thold;

    wparams.entropy_thold    = default_params.entropy_thold;
    wparams.logprob_thold    = default_params.logprob_thold;

    wparams.no_timestamps    = default_params.no_timestamps;
	    
	wparams.suppress_nst     = default_params.suppress_nst;

	wparams.vad            = default_params.vad;
	wparams.vad_model_path = default_params.vad_model.c_str();

	wparams.vad_params.threshold               = default_params.vad_threshold;
	wparams.vad_params.min_speech_duration_ms  = default_params.vad_min_speech_duration_ms;
	wparams.vad_params.min_silence_duration_ms = default_params.vad_min_silence_duration_ms;
	wparams.vad_params.max_speech_duration_s   = default_params.vad_max_speech_duration_s;
	wparams.vad_params.speech_pad_ms           = default_params.vad_speech_pad_ms;
	wparams.vad_params.samples_overlap         = default_params.vad_samples_overlap;
            
	// need minimum audio length
	if (pcmf32.size() < pcm_buffer_min)
	{
		pcmf32.insert(pcmf32.cend(), pcm_buffer_min - pcmf32.size(), 0.0f);
	}
	
	// we have a valid instance --> run recognition
	if (ctx)
	{
		std::cout << "Push audio to whisper, size=" << pcmf32.size() << std::endl;
		
#ifdef MEASURE_WHISPER_TIME
		auto start = std::chrono::high_resolution_clock::now();
#endif

		// this can degrade accuracy if n_processors > 1
		int whisper_call_result = whisper_full_parallel(ctx, wparams, pcmf32.data(), pcmf32.size(), default_params.n_processors);
		
#ifdef MEASURE_WHISPER_TIME
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

		std::cout << "whisper call took "	
              << duration.count()
              << " milliseconds\n";
#endif
		
		if (whisper_call_result != 0) 
		{
			// announce the error instead of crashing
			std::string errorText = getLocalTimeStamp().append(": Zmylk při spóznawanju. Spytajće prošu pozdźišo hišće raz.");
			// const char * text = "Zmylk při spóznawanju. Spytajće prošu pozdźišo hišće raz.";
			
			// TBD rather push an utterance than a token???
			RecognizedToken newResult(const_cast<char*>(errorText.c_str()), 5000, 200, 4800, 1.0f);
			tokens.push_back(newResult);
		}
		else
		{
			const int n_segments = whisper_full_n_segments(ctx);
			for (int i = 0; i < n_segments; ++i) {
				// const char * text = whisper_full_get_segment_text(ctx, i);
				int64_t t0 = 0;
				int64_t t1 = 0;
		
				// timestamps currently unused anyway?
				if (m_whisper_no_timestamps == false)
				{
					t0 = whisper_full_get_segment_t0(ctx, i);
					t1 = whisper_full_get_segment_t1(ctx, i);
				}
				
				float noSpeech = whisper_full_get_segment_no_speech_prob(ctx, i);
				// double logProbs = 0.0f;
				whisper_token_data token_data;
				
				// std::vector<float> tokenProbs;
				const int n_tokens = whisper_full_n_tokens(ctx, i);
				// fprintf(stderr,"tokens: %d\n",n_tokens);
				for (int j = 0; j < n_tokens; j++) {
					// FIXME read everything from token_data below???
					auto token = std::string(whisper_full_get_token_text(ctx, i, j));
					float probability = whisper_full_get_token_p(ctx, i, j);
					// std::cout << token << '\t' << probability << std::endl;
					// fprintf(stderr,"token: %s %f\n",token,probability);
					
					token_data = whisper_full_get_token_data(ctx, i, j);
					// logProbs += token_data.plog;
					
					// do not use probs from empty tokens and special tokens
					if (!token.empty() && token.front() != '[' && token.back() != ']')
					{
						// just collect all tokens
						RecognizedToken ntoken(const_cast<char*>(token.c_str()), 1000, 200, 800, probability, token_data.plog);
						tokens.push_back(ntoken);
						// tokenProbs.push_back(probability);
					}
					else
					{
						// std::cout << "Excluding token " << token << " from confidence!" << std::endl;	
					}
				}
				
				// logProbs /= n_tokens;

				std::cout << "#### no speech prob #### " << noSpeech << " %%%%%%%%%%%%%%" << std::endl;
				// std::cout << "#### avg logprob    #### " << logProbs << " %%%%%%%%%%%%%%" << std::endl;
				

				// TODO could eventually be used for confidence as well
				// float noSpeech = whisper_full_get_segment_no_speech_prob(ctx, i);
				// std::cout << "Segment " << i << '\t' << noSpeech << " no speech prob." << std::endl;
				
				// Compute mean
				
				/*
				float probSum = 0.0f;
				for (float val : tokenProbs) {
					probSum += val;
				}
				float probMean = probSum / tokenProbs.size();
				*/

				// Compute standard deviation
				/*
				float varianceSum = 0.0f;
				for (float val : tokenProbs) {
					varianceSum += (val - probMean) * (val - probMean);
				}
				float stddev = std::sqrt(varianceSum / tokenProbs.size()); // Population std dev
				*/
				
				// std::cout << "Sequence confidence: Mean = " << probMean << ", stddev = " << stddev << "." << std::endl;
				
				// std::unique_ptr<RecognitionResult> newResult = std::make_unique<RecognitionResult>(const_cast<char*>(text), (unsigned int) t0, (unsigned int) t1, (probMean - stddev));
				// partialResult.push_back(std::move(newResult));
			}
		}
	}
	else
	{
		// supply a dummy result
		std::string errorText = (getLocalTimeStamp().append(": System je přećežene. Spytajće prošu pozdźišo hišće raz."));
		// const char * text = "System je přećežene. Spytajće prošu pozdźišo hišće raz.";
		
		// TBD rather push an utterance than a token???
		RecognizedToken newResult(const_cast<char*>(errorText.c_str()), 5000, 200, 4800, 1.0f);
		tokens.push_back(newResult);
	}
}

//////////////////////////////////////////////
WhisperImpl::~WhisperImpl()
{
	whisper_free(ctx);
}

//////////////////////////////////////////////////////////////////////////////
std::string WhisperImpl::getLocalTimeStamp()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto str = oss.str();

    return str;
}
