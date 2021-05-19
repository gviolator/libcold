#pragma once

#include <cold/runtime/runtimeexport.h>
#include <cold/runtime/runtime.h>

#include <uv.h>


namespace cold {

struct RuntimeInternal
{
	static RUNTIME_EXPORT async::Scheduler::Ptr scheduler();

	static RUNTIME_EXPORT uv_loop_t* uv();
};

} // namespace cold

#define SWITCH_RUNTIME_SCHEDULER \
	if (!cold::Runtime::isRuntimeThread())\
	{\
		cold::async::Scheduler::Ptr scheduler__ = cold::RuntimeInternal::scheduler(); \
		DEBUG_CHECK(scheduler__)\
		co_await scheduler__;\
		DEBUG_CHECK(cold::Runtime::isRuntimeThread()) \
	}\
