
#include <SileroVadIterator.h>

//////////////////////////////////////////////////////////////////////////
// Loads the ONNX model.
void VadIterator::init_onnx_model(const std::string& model_path) {
	init_engine_threads(1, 1);
	session = std::make_shared<Ort::Session>(env, model_path.c_str(), session_options);
}

//////////////////////////////////////////////////////////////////////////
// Initializes threading settings.
void VadIterator::init_engine_threads(int inter_threads, int intra_threads) {
	session_options.SetIntraOpNumThreads(intra_threads);
	session_options.SetInterOpNumThreads(inter_threads);
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}

//////////////////////////////////////////////////////////////////////////
// Resets internal state (_state, _context, etc.)
void VadIterator::reset_states() {
	std::memset(_state.data(), 0, _state.size() * sizeof(float));
	triggered = false;
	temp_end = 0;
	current_sample = 0;
	prev_end = next_start = 0;
	current_speech = timestamp_t();
	std::fill(_context.begin(), _context.end(), 0.0f);
}

//////////////////////////////////////////////////////////////////////////
// Inference: runs inference on one chunk of input data.
// data_chunk is expected to have window_size_samples samples.
void VadIterator::predict(const std::vector<float>& data_chunk) {
	// Build new input: first context_samples from _context, followed by the current chunk (window_size_samples).
	std::vector<float> new_data(effective_window_size, 0.0f);
	std::copy(_context.begin(), _context.end(), new_data.begin());
	std::copy(data_chunk.begin(), data_chunk.end(), new_data.begin() + context_samples);
	input = new_data;

	// Create input tensor (input_node_dims[1] is already set to effective_window_size).
	Ort::Value input_ort = Ort::Value::CreateTensor<float>(
		memory_info, input.data(), input.size(), input_node_dims, 2);
	Ort::Value state_ort = Ort::Value::CreateTensor<float>(
		memory_info, _state.data(), _state.size(), state_node_dims, 3);
	Ort::Value sr_ort = Ort::Value::CreateTensor<int64_t>(
		memory_info, sr.data(), sr.size(), sr_node_dims, 1);
	ort_inputs.clear();
	ort_inputs.emplace_back(std::move(input_ort));
	ort_inputs.emplace_back(std::move(state_ort));
	ort_inputs.emplace_back(std::move(sr_ort));

	// Run inference.
	ort_outputs = session->Run(
		Ort::RunOptions{ nullptr },
		input_node_names.data(), ort_inputs.data(), ort_inputs.size(),
		output_node_names.data(), output_node_names.size());

	float speech_prob = ort_outputs[0].GetTensorMutableData<float>()[0];
	float* stateN = ort_outputs[1].GetTensorMutableData<float>();
	std::memcpy(_state.data(), stateN, size_state * sizeof(float));
	current_sample += static_cast<unsigned int>(window_size_samples); // Advance by the original window size.

	// If speech is detected (probability >= threshold)
	if (speech_prob >= threshold) {
#ifdef __DEBUG_SPEECH_PROB___
		float speech = current_sample - window_size_samples;
		printf("{ start: %.3f s (%.3f) %08d}\n", 1.0f * speech / sample_rate, speech_prob, current_sample - window_size_samples);
#endif
		if (temp_end != 0) {
			temp_end = 0;
			if (next_start < prev_end)
				next_start = current_sample - window_size_samples;
		}
		if (!triggered) {
			triggered = true;
			current_speech.start = current_sample - window_size_samples;
		}
		// Update context: copy the last context_samples from new_data.
		std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
		return;
	}

	// If the speech segment becomes too long.
	if (triggered && ((current_sample - current_speech.start) > max_speech_samples)) {
		if (prev_end > 0) {
			current_speech.end = prev_end;
			current_speech = timestamp_t();
			if (next_start < prev_end)
				triggered = false;
			else
				current_speech.start = next_start;
			prev_end = 0;
			next_start = 0;
			temp_end = 0;
		}
		else {
			current_speech.end = current_sample;
			current_speech = timestamp_t();
			prev_end = 0;
			next_start = 0;
			temp_end = 0;
			triggered = false;
		}
		std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
		return;
	}

	if ((speech_prob >= (threshold - 0.15)) && (speech_prob < threshold)) {
		// When the speech probability temporarily drops but is still in speech, update context without changing state.
		std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
		return;
	}

	if (speech_prob < (threshold - 0.15)) {
#ifdef __DEBUG_SPEECH_PROB___
		float speech = current_sample - window_size_samples - speech_pad_samples;
		printf("{ end: %.3f s (%.3f) %08d}\n", 1.0f * speech / sample_rate, speech_prob, current_sample - window_size_samples);
#endif
		if (triggered) {
			if (temp_end == 0)
				temp_end = current_sample;
			if (current_sample - temp_end > min_silence_samples_at_max_speech)
				prev_end = temp_end;
			if ((current_sample - temp_end) >= min_silence_samples) {
				current_speech.end = temp_end;
				if (current_speech.end - current_speech.start > min_speech_samples) {
					current_speech = timestamp_t();
					prev_end = 0;
					next_start = 0;
					temp_end = 0;
					triggered = false;
				}
			}
		}
		std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
		return;
	}
}

//////////////////////////////////////////////////////////////////////////
// Public method to reset the internal state.
void VadIterator::reset() {
	reset_states();
}
