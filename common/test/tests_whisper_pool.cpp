
#include "doctest.h"

#include <WhisperPool.h>

#include <stddef.h>
#include <stdint.h>

TEST_CASE("simple allocate/deallocate tests")
{	
	SUBCASE("pool of one") {
		whisper_mock_set_overload(false, false);
		WhisperPool::allocate(1);
		std::unique_ptr<WhisperImpl> inst;
		inst = WhisperPool::getInstance();
		WhisperPool::releaseInstance(std::move(inst));
	}
	
	

}

