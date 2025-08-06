
#ifndef SILERO_VAD_INTERATOR_H
#define SILERO_VAD_INTERATOR_H

#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <limits>
#include <chrono>
#include <iomanip>
#include <memory>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <cmath>    // for std::rint
#if __cplusplus < 201703L
#include <memory>
#endif

//#define __DEBUG_SPEECH_PROB___

#include "onnxruntime_cxx_api.h"

class timestamp_t {
public:
    int start;
    int end;

    timestamp_t(int start = -1, int end = -1)
        : start(start), end(end) { }

    timestamp_t& operator=(const timestamp_t& a) {
        start = a.start;
        end = a.end;
        return *this;
    }

    bool operator==(const timestamp_t& a) const {
        return (start == a.start && end == a.end);
    }
};

// VadIterator class: uses ONNX Runtime to detect speech segments.
class VadIterator {
private:
    // ONNX Runtime resources
    Ort::Env env;
    Ort::SessionOptions session_options;
    std::shared_ptr<Ort::Session> session = nullptr;
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU);

    // ----- Context-related additions -----
    const int context_samples = 64;  // For 16kHz, 64 samples are added as context.
    std::vector<float> _context;     // Holds the last 64 samples from the previous chunk (initialized to zero).

    // Original window size (e.g., 32ms corresponds to 512 samples)
    int window_size_samples;
    // Effective window size = window_size_samples + context_samples
    int effective_window_size;

    // Additional declaration: samples per millisecond
    int sr_per_ms;

    // ONNX Runtime input/output buffers
    std::vector<Ort::Value> ort_inputs;
    std::vector<const char*> input_node_names = { "input", "state", "sr" };
    std::vector<float> input;
    unsigned int size_state = 2 * 1 * 128;
    std::vector<float> _state;
    std::vector<int64_t> sr;
    int64_t input_node_dims[2] = {};
    const int64_t state_node_dims[3] = { 2, 1, 128 };
    const int64_t sr_node_dims[1] = { 1 };
    std::vector<Ort::Value> ort_outputs;
    std::vector<const char*> output_node_names = { "output", "stateN" };

    // Model configuration parameters
    int sample_rate;
    float threshold;
    int min_silence_samples;
    int min_silence_samples_at_max_speech;
    int min_speech_samples;
    float max_speech_samples;
    int speech_pad_samples;
    int audio_length_samples;

    // State management
    bool triggered = false;
    unsigned int temp_end = 0;
    unsigned int current_sample = 0;
    int prev_end;
    int next_start = 0;
    timestamp_t current_speech;

    void init_onnx_model(const std::string& model_path);
    void init_engine_threads(int inter_threads, int intra_threads);
    void reset_states();
    
public:
    void reset();
    void predict(const std::vector<float>& data_chunk);
    bool getTriggered(void) { return triggered; }
    
    VadIterator(const std::string ModelPath,
        int Sample_rate = 16000, int windows_frame_size = 32,
        float Threshold = 0.5, int min_silence_duration_ms = 100,
        int speech_pad_ms = 30, int min_speech_duration_ms = 250,
        float max_speech_duration_s = std::numeric_limits<float>::infinity())
        : sample_rate(Sample_rate), threshold(Threshold), speech_pad_samples(speech_pad_ms), prev_end(0)
    {
        sr_per_ms = sample_rate / 1000;  // e.g., 16000 / 1000 = 16
        window_size_samples = windows_frame_size * sr_per_ms; // e.g., 32ms * 16 = 512 samples
        effective_window_size = window_size_samples + context_samples; // e.g., 512 + 64 = 576 samples
        input_node_dims[0] = 1;
        input_node_dims[1] = effective_window_size;
        _state.resize(size_state);
        sr.resize(1);
        sr[0] = sample_rate;
        _context.assign(context_samples, 0.0f);
        min_speech_samples = sr_per_ms * min_speech_duration_ms;
        max_speech_samples = (sample_rate * max_speech_duration_s - window_size_samples - 2 * speech_pad_samples);
        min_silence_samples = sr_per_ms * min_silence_duration_ms;
        min_silence_samples_at_max_speech = sr_per_ms * 98;
        init_onnx_model(ModelPath);
    }

};

#endif
