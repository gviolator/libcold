#include "pch.h"
#include "runtimescopeguard.h"
#include <cold/runtime/runtime.h>
#include <cold/runtime/internal/runtimeinternal.h>
#include <cold/threading/event.h>
#include <cold/diagnostics/logging.h>


using namespace cold;
using namespace testing;




TEST(Test_Runtime, Test1)
{
	constexpr size_t DefaultWorksCount = 100;
	constexpr size_t PoolWorksCount = 500;

	size_t counter = 0;
	std::atomic_size_t poolCounter = 0;

	{
		RuntimeScopeGuard runtimeGuard;

		for (size_t i = 0; i < DefaultWorksCount; ++i )
		{

			RuntimeInternal::scheduler()->schedule([](void* p1, void* counter) noexcept
				{
				
				++(*reinterpret_cast<size_t*>(counter));

				if ((uint64_t)p1 == DefaultWorksCount - 1)
				{
					Runtime::instance().stop();
				}

			}, (void*)i, &counter);
		}

		for (size_t i = 0; i < PoolWorksCount; ++i )
		{

			Runtime::instance().poolScheduler()->schedule([](void* p1, void* counter) noexcept
			{
				reinterpret_cast<std::atomic_size_t*>(counter)->fetch_add(1);
			}, (void*)i, &poolCounter);
		}
	}

	ASSERT_THAT(counter, Eq(DefaultWorksCount));
	ASSERT_THAT(poolCounter, Eq(PoolWorksCount));
}
