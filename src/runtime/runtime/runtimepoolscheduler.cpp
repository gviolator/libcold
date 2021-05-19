#include "pch.h"

#include "cold/runtime/runtime.h"
#include "cold/runtime/internal/runtimeallocator.h"
#include "cold/runtime/internal/uvutils.h"
#include "cold/runtime/internal/runtimecomponent.h"
#include "cold/async/scheduler.h"

#include "cold/com/comclass.h"
#include "cold/memory/rtstack.h"
#include "cold/utils/scopeguard.h"
#include "cold/threading/setthreadname.h"
#include "cold/threading/critical_section.h"

#include "componentids.h"

#define ASSIGN_POOLTHREAD_NAME


using namespace cold::async;
using namespace cold::atd_literals;

namespace cold {

class RuntimePoolScheduler;

namespace {

#ifdef ASSIGN_POOLTHREAD_NAME

unsigned threadPoolThreadNo()
{
	static std::atomic<unsigned> counter = 1;
	return counter.fetch_add(1);
}

struct PoolThreadAutoName
{
	PoolThreadAutoName()
	{
		const unsigned threadNo = threadPoolThreadNo();
		threading::setCurrentThreadName(format("Pool Thread-%1", threadNo).c_str());
	}
};

#endif


struct Work : uv_work_t
{
	RuntimePoolScheduler* scheduler;
	Scheduler::Invocation invocation;

	Work(RuntimePoolScheduler* scheduler_, Scheduler::Invocation&& invocation_): scheduler(scheduler_), invocation(std::move(invocation_))
	{}
};

}


class RuntimePoolScheduler final : public Scheduler, public RuntimeComponent
{
	COMCLASS_(Scheduler, RuntimeComponent)

	static constexpr size_t MaxProcessedWorks = 40;

public:

	RuntimePoolScheduler(uv_loop_t* uv_)
	{
		uv_async_init(uv_, m_async, [](uv_async_t* handle) noexcept
		{
			RuntimePoolScheduler& this_ = *reinterpret_cast<RuntimePoolScheduler*>(handle->data);
			this_.scheduleNextWorks() ;
		});

		uv_handle_set_data(m_async, this);
	}

	~RuntimePoolScheduler()
	{}

	void scheduleInvocation(Invocation invocation) noexcept override
	{
		if (Runtime::isRuntimeThread())
		{
			if (m_works.size() < MaxProcessedWorks)
			{
				Work& work = m_works.emplace_back(this, std::move(invocation));
				scheduleWork(work);
			}
			else
			{
				std::lock_guard lock{m_mutex};
				m_invocations.emplace_back(std::move(invocation));
			}
		}
		else
		{
			{
				std::lock_guard lock{m_mutex};
				m_invocations.emplace_back(std::move(invocation));
			}

			uv_async_send(m_async);
		}
	}

private:

	void scheduleWork(Work& work) noexcept
	{
		uv_queue_work(m_async->loop, &work, [](uv_work_t* const req) noexcept
		{
#ifdef ASSIGN_POOLTHREAD_NAME
			[[maybe_unused]] thread_local static PoolThreadAutoName threadAutoName;
#endif

			thread_local static rtstack(2_Mb);

			auto& work = *static_cast<Work*>(req);
			RuntimePoolScheduler& scheduler = *work.scheduler;

			Scheduler::InvocationGuard invocation{scheduler};
			Scheduler::invoke(scheduler, std::move(work.invocation));
		},

		[](uv_work_t* const req, int status) noexcept
		{
			auto& work = *static_cast<Work*>(req);
			RuntimePoolScheduler& this_ = *work.scheduler;

			auto iter = std::find_if(this_.m_works.begin(), this_.m_works.end(), [workPtr = &work](const Work& work) {
				return workPtr == &work;
			});

			DEBUG_CHECK(iter != this_.m_works.end())

			this_.m_works.erase(iter);
			this_.scheduleNextWorks();
		});
	}

	void scheduleNextWorks() noexcept
	{
		if (m_works.size() >= MaxProcessedWorks)
		{
			return;
		}

		std::pmr::list<Invocation> invocations{runtimePoolMemoryResource()};

		{
			std::lock_guard lock{m_mutex};

			if (!m_invocations.empty())
			{
				const size_t queueInvocationsCount = std::min(m_invocations.size(), MaxProcessedWorks - m_works.size());

				auto lastPendindInvocation = m_invocations.begin();
				std::advance(lastPendindInvocation, queueInvocationsCount);
				invocations.splice(invocations.begin(), m_invocations, m_invocations.begin(), lastPendindInvocation);
			}
		}

		if (invocations.empty())
		{
			return;
		}

		for (Scheduler::Invocation& invocation : invocations)
		{
			Work& work = m_works.emplace_back(this, std::move(invocation));
			scheduleWork(work);
		}
	}

	uv_loop_t* uvLoop() const
	{
		return m_async->loop;
	}

	void waitAnyActivity() noexcept override 
	{
		RUNTIME_FAILURE("Unexpected ::waitAnyActivity invocation")
	}

	bool componentHasWorks() override
	{
		std::lock_guard lock{m_mutex};
		return !(m_invocations.empty() && m_works.empty());
	}



	UvHandle<uv_async_t> m_async;
	threading::CriticalSection m_mutex;
	std::pmr::list<Invocation> m_invocations {runtimePoolMemoryResource()};
	std::pmr::list<Work> m_works {runtimePoolMemoryResource()};
	std::atomic_uint m_processedWorks{0};
};


namespace {


const auto registration = RuntimeComponent::registerComponent(ComponentIds::PoolScheduler, [](uv_loop_t* uv, std::string_view, void*) noexcept -> RuntimeComponent::Ptr
{
	return com::createInstanceSingleton<RuntimePoolScheduler, RuntimeComponent>(uv);

}, nullptr);

}

}
