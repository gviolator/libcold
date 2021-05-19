#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/com/comptr.h>
#include <cold/async/scheduler.h>

#include <memory>


namespace cold {


/**
* 
*/
struct ABSTRACT_TYPE Runtime
{
	enum class RunMode
	{
		Default,
		NoWait
	};

	RUNTIME_EXPORT static std::unique_ptr<Runtime> create();

	RUNTIME_EXPORT static bool exists();

	RUNTIME_EXPORT static Runtime& instance();

	RUNTIME_EXPORT static bool isRuntimeThread();

	virtual ~Runtime() = default;

	/**
	* 
	*/
	virtual void run(RunMode = RunMode::Default) = 0;

	virtual void stop() = 0;
	// virtual async::Scheduler::Ptr scheduler() = 0;
	virtual async::Scheduler::Ptr poolScheduler() const = 0;

	virtual ComPtr<> findComponent(std::string_view id) const = 0;
};

}
