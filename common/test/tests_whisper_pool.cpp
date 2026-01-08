
#include "doctest.h"

#include <WhisperPool.h>

#include <stddef.h>
#include <stdint.h>

#include <thread>
#include <chrono>

#include <iostream>

TEST_CASE("simple allocate/deallocate tests")
{	
	
	SUBCASE("pool of one") {
		whisper_mock_set_overload(false, false);
		WhisperPool::setWhisperParams("ggml.bin", "auto", -1, false, true, false);
		WhisperPool::allocate(1);
		std::unique_ptr<WhisperImpl> inst;
		inst = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst));
		WhisperPool::unregister();
	}
	
	SUBCASE("pool of two") {
		whisper_mock_set_overload(false, false);
		WhisperPool::setWhisperParams("ggml.bin", "auto", -1, false, true, false);
		WhisperPool::allocate(2);
		std::unique_ptr<WhisperImpl> inst1, inst2;
		inst1 = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst1));
		inst1 = WhisperPool::getInstance();
		inst2 = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst1));
		WhisperPool::releaseInstance(std::move(inst2));
		inst1 = WhisperPool::getInstance();
		inst2 = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst2));
		WhisperPool::releaseInstance(std::move(inst1));
		WhisperPool::unregister();
	}
}

void concurrent_user() {
	std::unique_ptr<WhisperImpl> inst;
	std::cout << "concurrent_user: allocate" << std::endl;
	inst = WhisperPool::getInstance();
	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "concurrent_user: release" << std::endl;
	WhisperPool::releaseInstance(std::move(inst));
};
	

TEST_CASE("try overallocation")
{	
	SUBCASE("overallocate one") {
		whisper_mock_set_overload(false, false);
		WhisperPool::setWhisperParams("ggml.bin", "auto", -1, false, true, false);
		WhisperPool::allocate(1);
		std::thread t(concurrent_user);
		std::unique_ptr<WhisperImpl> inst;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		inst = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst));
		t.join();
		WhisperPool::unregister();
	}
	
	SUBCASE("overallocate two") {
		whisper_mock_set_overload(false, false);
		WhisperPool::setWhisperParams("ggml.bin", "auto", -1, false, true, false);
		WhisperPool::allocate(1);
		std::thread t1(concurrent_user);
		std::thread t2(concurrent_user);
		std::unique_ptr<WhisperImpl> inst;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		inst = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst));
		t2.join();
		t1.join();
		WhisperPool::unregister();
	}
	
}

void concurrent_user_allocate() {
	WhisperPool::allocate(1);
	std::unique_ptr<WhisperImpl> inst;
	std::cout << "concurrent_user: allocate" << std::endl;
	inst = WhisperPool::getInstance();
	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "concurrent_user: release" << std::endl;
	WhisperPool::releaseInstance(std::move(inst));
	WhisperPool::unregister();
};
	

TEST_CASE("try overallocation with dedicated allocation")
{	
	SUBCASE("overallocate one") {
		whisper_mock_set_overload(false, false);
		WhisperPool::setWhisperParams("ggml.bin", "auto", -1, false, true, false);
		WhisperPool::allocate(1);
		std::thread t(concurrent_user);
		std::unique_ptr<WhisperImpl> inst;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		inst = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst));
		t.join();
		WhisperPool::unregister();
	}
	
	SUBCASE("overallocate two") {
		whisper_mock_set_overload(false, false);
		WhisperPool::setWhisperParams("ggml.bin", "auto", -1, false, true, false);
		WhisperPool::allocate(1);
		std::thread t1(concurrent_user);
		std::thread t2(concurrent_user);
		std::unique_ptr<WhisperImpl> inst;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		inst = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst));
		t2.join();
		t1.join();
		WhisperPool::unregister();
	}
	
}


