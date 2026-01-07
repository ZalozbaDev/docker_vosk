
#include "doctest.h"

#include <WhisperPool.h>

#include <stddef.h>
#include <stdint.h>

TEST_CASE("simple allocate/deallocate tests")
{	
	SUBCASE("pool of one") {
		whisper_mock_set_overload(false, false);
		WhisperPool::setWhisperParams("ggml.bin", "auto", -1, false, true, false);
		WhisperPool::allocate(1);
		std::unique_ptr<WhisperImpl> inst;
		inst = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst));
		WhisperPool::allocate(0);
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
		WhisperPool::allocate(0);
	}
	
	

}

