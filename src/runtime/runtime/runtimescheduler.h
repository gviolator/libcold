#pragma once
#include "cold/async/scheduler.h"
#include "cold/com/interface.h"
#include "cold/meta/classinfo.h"
#include <uv.h>

namespace cold {

/**
*/
class ABSTRACT_TYPE RuntimeScheduler : public async::Scheduler
{
	CLASS_INFO(
		CLASS_BASE(async::Scheduler)
	)

	virtual bool hasPendingTasks() = 0;

protected:
	virtual uv_loop_t* uvLoop() const = 0;
	

private:

	virtual void waitAnyActivity() noexcept final;
};

}
