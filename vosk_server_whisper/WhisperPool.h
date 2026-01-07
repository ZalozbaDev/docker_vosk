#ifndef WHISPER_POOL_H
#define WHISPER_POOL_H

#include <thread>
#include <mutex>
#include <condition_variable>

#include "WhisperImpl.h"

class WhisperPool
{
public:
	static void setWhisperParams(std::string modelPath, std::string vosk_model_language, int whisper_max_context, bool whisper_no_timestamps, bool whisper_no_fallback, bool whisper_force_cpu);
	static void allocate(std::size_t size);
	static std::unique_ptr<WhisperImpl> getInstance(void);
	static void releaseInstance(std::unique_ptr<WhisperImpl> inst);
	static void unregister();
	~WhisperPool();
private:
	WhisperPool();
	static std::mutex instance_mutex;
	static std::vector<std::unique_ptr<WhisperImpl>> instances;
	static std::condition_variable instance_notify;
	
	static int number_users;
	static std::mutex user_mutex;
	
	static std::string m_modelPath;
	
	static std::string m_vosk_model_language;
	static int m_whisper_max_context;
	static bool m_whisper_no_timestamps;
	static bool m_whisper_no_fallback;
	static bool m_whisper_force_cpu;
};


#endif // WHISPER_POOL_H
