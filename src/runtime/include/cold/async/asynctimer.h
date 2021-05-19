#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/async/taskbasetypes.h>
#include <cold/com/ianything.h>
#include <cold/memory/allocator.h>

#include <chrono>
#include <optional>

namespace cold::async {

/**
*/
struct ABSTRACT_TYPE AsyncTimer : virtual IRefCounted
{
	using Ptr = ComPtr<AsyncTimer>;

	virtual ~AsyncTimer() = default;
	virtual Task<bool> tick() = 0;
	virtual void stop();
};

/**
*/
RUNTIME_EXPORT AsyncTimer::Ptr createRuntimeTimer(std::chrono::milliseconds dueTime, std::optional<std::chrono::milliseconds> repeat = std::nullopt, ComPtr<Allocator> = {});

}

namespace cold::async_internal {

/**
*/
RUNTIME_EXPORT void runtimeResumeAfter(std::chrono::milliseconds, async::Scheduler::Ptr, std::coroutine_handle<>) noexcept;

}
