#pragma once

#if 0
#include "com/comclass.h"
#include "runtime/runtimeexport.h"
#include "async/scheduler.h"

#include "utils/uid.h"
#include "com/interface.h"
//#include "sys/moduleinfo.h"


namespace cold::async {

class Scheduler;

namespace core {

/**
*/
struct INTERFACE_API ISchedulerGuard
{
	virtual ~ISchedulerGuard() = default;

	virtual Uid uid() const noexcept = 0;

	RUNTIME_EXPORT static std::unique_ptr<ISchedulerGuard> create(Scheduler::Ptr);
};

/// <summary>
///
/// </summary>
template<typename T, typename ... Args>
std::unique_ptr<core::ISchedulerGuard> makeSchedulerGuard(Args&& ... args) {
	static_assert(std::is_base_of_v<cold::async::Scheduler, T>, "Type must be a Scheduler");

	auto scheduler = com::createInstance<T>(std::forward<Args>(args)...);

	return ISchedulerGuard::create(std::move(scheduler));
}

} // namespace async_internal


using SchedulerGuard = std::unique_ptr<core::ISchedulerGuard>;


} // namespace cold::async

#endif