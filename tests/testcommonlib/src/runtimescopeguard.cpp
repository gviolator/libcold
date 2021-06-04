#include "pch.h"
#include "runtimescopeguard.h"
#include <cold/runtime/runtime.h>
#include <cold/threading/event.h>
#include <cold/threading/setthreadname.h>
#include <cold/utils/scopeguard.h>
#include <cold/memory/rtstack.h>


using namespace cold;
using namespace cold::cold_literals;

RuntimeScopeGuard::RuntimeScopeGuard()
{
	threading::Event signal;

	SCOPE_Leave {
		signal.wait();
	};

	thread = std::thread([&signal]
	{
		threading::setCurrentThreadName("Runtime Thread");
		auto runtime = Runtime::create();
		async::Scheduler::setDefault(Runtime::instance().poolScheduler());

		SCOPE_Leave {
			async::Scheduler::setDefault({});
		};

		signal.set();

		rtstack(1_Mb);

		runtime->run();
	});
}

RuntimeScopeGuard::~RuntimeScopeGuard()
{
	reset();
}

void RuntimeScopeGuard::reset()
{
	if (Runtime::exists())
	{
		Runtime::instance().stop();
	}

	if (thread.joinable())
	{
		thread.join();
	}

}
