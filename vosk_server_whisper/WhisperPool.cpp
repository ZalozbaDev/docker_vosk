#include "WhisperPool.h"

#include <iostream>

std::mutex WhisperPool::instance_mutex;
std::vector<std::unique_ptr<WhisperImpl>> WhisperPool::instances;
std::condition_variable WhisperPool::instance_notify;
	
std::string WhisperPool::m_modelPath;
	
std::string WhisperPool::m_vosk_model_language;
int WhisperPool::m_whisper_max_context;
bool WhisperPool::m_whisper_no_timestamps;
bool WhisperPool::m_whisper_no_fallback;
bool WhisperPool::m_whisper_force_cpu;
bool WhisperPool::m_whisper_translate_mode;

int WhisperPool::number_users = 0;
std::mutex WhisperPool::user_mutex;

//////////////////////////////////////////////
WhisperPool::WhisperPool()
{
}

//////////////////////////////////////////////
void WhisperPool::setWhisperParams(std::string modelPath, std::string vosk_model_language, int whisper_max_context, bool whisper_no_timestamps, bool whisper_no_fallback, bool whisper_force_cpu, bool whisper_translate_mode)
{
	m_modelPath = modelPath;
	
	m_vosk_model_language    = vosk_model_language;
	m_whisper_max_context    = whisper_max_context;
	m_whisper_no_timestamps  = whisper_no_timestamps;
	m_whisper_no_fallback    = whisper_no_fallback;
	m_whisper_force_cpu      = whisper_force_cpu;
	m_whisper_translate_mode = whisper_translate_mode;
}

//////////////////////////////////////////////
void WhisperPool::allocate(std::size_t size)
{
	std::unique_lock<std::mutex> users_lock{user_mutex};
	
	number_users++;

	std::cout << "WhisperPool::allocate: total users = " << number_users << "." << std::endl;
	
	if (instances.size() != size)
	{
		std::unique_lock<std::mutex> instances_lock{instance_mutex};
		
		if (instances.size() > size)
		{
			while (instances.size() > size)
			{
				instances.pop_back();
				std::cout << "WhisperPool::allocate: REMOVE, total instances = " << instances.size() << "." << std::endl;
			}
		}
		else
		{
			while (instances.size() < size)
			{
				std::unique_ptr<WhisperImpl> inst = std::make_unique<WhisperImpl>(m_modelPath, 
					m_vosk_model_language, m_whisper_max_context, m_whisper_no_timestamps, 
					m_whisper_no_fallback, m_whisper_force_cpu, m_whisper_translate_mode);		
				instances.push_back(std::move(inst));
				
				std::cout << "WhisperPool::allocate: GROW, total instances = " << instances.size() << "." << std::endl;
			}
		}
		
	}
	
}

//////////////////////////////////////////////
std::unique_ptr<WhisperImpl> WhisperPool::getInstance(void)
{
	std::unique_lock<std::mutex> instances_lock{instance_mutex};
	
	while (true)
	{
		if (instances.size() > 0)
		{
			std::unique_ptr<WhisperImpl> inst = std::move(instances.back());
			instances.pop_back();
			std::cout << "WhisperPool::getInstance: remaining = " << instances.size() << "." << std::endl;
			return inst;
		}
		else
		{
			std::cout << "WhisperPool::getInstance: waiting for other user to release instance." << std::endl;
			instance_notify.wait(instances_lock);
			std::cout << "WhisperPool::getInstance: notify() for newly available instance." << std::endl;
		}
	}
}

//////////////////////////////////////////////
void WhisperPool::releaseInstance(std::unique_ptr<WhisperImpl> inst)
{
	std::unique_lock<std::mutex> instances_lock{instance_mutex};

	instances.push_back(std::move(inst));
	
	instances_lock.unlock();

	std::cout << "WhisperPool::releaseInstance: available = " << instances.size() << "." << std::endl;
	
	instance_notify.notify_one();
}

//////////////////////////////////////////////
void WhisperPool::unregister()
{
	std::unique_lock<std::mutex> users_lock{user_mutex};
	
	number_users--;

	std::cout << "WhisperPool::unregister: total users = " << number_users << "." << std::endl;
	
	if (number_users == 0)
	{
		while (instances.size() > 0)
		{
			instances.pop_back();
			std::cout << "WhisperPool::unregister: REMOVE, total instances = " << instances.size() << "." << std::endl;
		}
	}
}

//////////////////////////////////////////////
WhisperPool::~WhisperPool()
{
}
