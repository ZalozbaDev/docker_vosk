
// Author      : NathanJHLee
// Created On  : 2025-11-10
// Description : silero 6.2 system for onnx-runtime(c++) and torch-script(c++)
// Version     : 1.3

#include <SileroVadIterator.h>

namespace silero {

		//////////////////////////////////////////////////////////////////////////
		// DS: Public method to reset the internal state.
		void VadIterator::reset() {
			reset_states();
		}

        VadIterator::VadIterator(const std::string &model_path, 
			float threshold, 
			int sample_rate, 
			int window_size_ms, 
			int speech_pad_ms, 
			int min_silence_duration_ms, 
			int min_speech_duration_ms, 
			int max_duration_merge_ms, 
			bool print_as_samples)
                :sample_rate(sample_rate), threshold(threshold), window_size_ms(window_size_ms), 
		speech_pad_ms(speech_pad_ms), min_silence_duration_ms(min_silence_duration_ms), 
		min_speech_duration_ms(min_speech_duration_ms), max_duration_merge_ms(max_duration_merge_ms), 
		print_as_samples(print_as_samples),
                env(ORT_LOGGING_LEVEL_ERROR, "Vad"), session_options(), session(nullptr), allocator(), 
		memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU)), context_samples(64), 
		_context(64, 0.0f), current_sample(0), size_state(2 * 1 * 128), 
		input_node_names({"input", "state", "sr"}), output_node_names({"output", "stateN"}), 
		state_node_dims{2, 1, 128}, sr_node_dims{1}

        {
                init_onnx_model(model_path);
        }
        VadIterator::~VadIterator(){
        }

        void VadIterator::init_onnx_model(const std::string& model_path) {
                int inter_threads=1;
                int intra_threads=1;
                session_options.SetIntraOpNumThreads(intra_threads);
                session_options.SetInterOpNumThreads(inter_threads);
                session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
                session = std::make_shared<Ort::Session>(env, model_path.c_str(), session_options);
		std::cout<<"Silero onnx-Model loaded successfully"<<std::endl;
        }

        float VadIterator::predict(const std::vector<float>& data_chunk) {
                // _context와 현재 청크를 결합하여 입력 데이터 구성
                std::vector<float> new_data(effective_window_size, 0.0f);
                std::copy(_context.begin(), _context.end(), new_data.begin());
                std::copy(data_chunk.begin(), data_chunk.end(), new_data.begin() + context_samples);
                input = new_data;

		Ort::Value input_ort = Ort::Value::CreateTensor<float>(
                                memory_info, input.data(), input.size(), input_node_dims, 2);
                Ort::Value state_ort = Ort::Value::CreateTensor<float>(
                                memory_info, _state.data(), _state.size(), state_node_dims, 3);
                Ort::Value sr_ort = Ort::Value::CreateTensor<int64_t>(
                                memory_info, sr.data(), sr.size(), sr_node_dims, 1);
                ort_inputs.clear();
                ort_inputs.push_back(std::move(input_ort));
                ort_inputs.push_back(std::move(state_ort));
                ort_inputs.push_back(std::move(sr_ort));

                ort_outputs = session->Run(
                                Ort::RunOptions{ nullptr },
                                input_node_names.data(), ort_inputs.data(), ort_inputs.size(),
                                output_node_names.data(), output_node_names.size());

                float speech_prob = ort_outputs[0].GetTensorMutableData<float>()[0]; // ONNX 출력: 첫 번째 값이 음성 확률

                float* stateN = ort_outputs[1].GetTensorMutableData<float>(); // 두 번째 출력값: 상태 업데이트
                std::memcpy(_state.data(), stateN, size_state * sizeof(float));

                std::copy(new_data.end() - context_samples, new_data.end(), _context.begin());
                // _context 업데이트: new_data의 마지막 context_samples 유지

                return speech_prob;
        }

        void VadIterator::reset_states() {
                triggered = false;
                current_sample = 0;
                temp_end = 0;
                outputs_prob.clear();
                total_sample_size = 0;
                std::memset(_state.data(), 0, _state.size() * sizeof(float));
                std::fill(_context.begin(), _context.end(), 0.0f);
        }

        void VadIterator::init_engine(int window_size_ms) {
                min_silence_samples = sample_rate * min_silence_duration_ms / 1000;
                speech_pad_samples = sample_rate * speech_pad_ms / 1000;
                window_size_samples = sample_rate / 1000 * window_size_ms;
                min_speech_samples = sample_rate * min_speech_duration_ms / 1000;

                //for ONNX
                context_samples=window_size_samples / 8;
                _context.assign(context_samples, 0.0f);

                effective_window_size = window_size_samples + context_samples;  // 예: 512 + 64 = 576 samples
                input_node_dims[0] = 1;
                input_node_dims[1] = effective_window_size;
                _state.resize(size_state);
                sr.resize(1);
                sr[0] = sample_rate;

        }

}
