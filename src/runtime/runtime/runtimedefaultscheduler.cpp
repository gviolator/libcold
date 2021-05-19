#include "pch.h"
#include "cold/runtime/internal/runtimeallocator.h"
// #include "runtimescheduler.h"
#include "cold/runtime/internal/uvutils.h"
#include "cold/runtime/internal/runtimecomponent.h"
#include "cold/async/scheduler.h"

#include "cold/com/comclass.h"
#include "cold/utils/scopeguard.h"
#include "cold/threading/critical_section.h"

#include "componentids.h"


using namespace cold::async;

namespace cold {



class RuntimeDefaultScheduler final : public Scheduler, public RuntimeComponent
{
	COMCLASS_(Scheduler, RuntimeComponent)

public:

	RuntimeDefaultScheduler(uv_loop_t* uv_)
	{
		uv_async_init(uv_, m_async, [](uv_async_t* handle) noexcept
		{
			RuntimeDefaultScheduler& this_ = *reinterpret_cast<RuntimeDefaultScheduler*>(handle->data);
			this_.processInvocations();
		});

		uv_handle_set_data(m_async, this);
	}

	~RuntimeDefaultScheduler()
	{}

	void scheduleInvocation(Invocation invocation) noexcept override
	{
		{
			std::lock_guard lock{m_mutex};
			m_invocations.emplace_back(std::move(invocation));
		}

		uv_async_send(m_async);
	}


private:

	void processInvocations() noexcept
	{
		Scheduler::InvocationGuard guard{*this};

		while (true)
		{
			std::pmr::list<Invocation> invocations(runtimePoolMemoryResource());

			{
				std::lock_guard lock{m_mutex};
				if (m_invocations.empty())
				{
					break;
				}

				m_workInProgress = true;
				invocations.splice(invocations.begin(), m_invocations);
			}

			SCOPE_Leave {
				std::lock_guard lock{m_mutex};
				m_workInProgress = false;
			};

			for (auto& invocation : invocations)
			{
				Scheduler::invoke(*this, std::move(invocation));
			}
		}
	}

	uv_loop_t* uvLoop() const
	{
		return m_async->loop;
	}

	void waitAnyActivity() noexcept override 
	{
		RUNTIME_FAILURE("Unexpected ::waitAnyActivity invocation")
		//uv_loop_t* const uv = this->uvLoop();
		//while (this->hasWorks())
		//{
		//	uv_run(uv, UV_RUN_ONCE);
		//}
	}

	bool componentHasWorks() override
	{
		std::lock_guard lock{m_mutex};
		return !m_invocations.empty() || m_workInProgress;
	}

	UvHandle<uv_async_t> m_async;
	threading::CriticalSection m_mutex;
	std::pmr::list<Scheduler::Invocation> m_invocations{runtimePoolMemoryResource()};
	bool m_workInProgress = false;
};


namespace {


const auto registration = RuntimeComponent::registerComponent(ComponentIds::DefaultScheduler, [](uv_loop_t* uv, std::string_view, void*) noexcept -> RuntimeComponent::Ptr
{
	return com::createInstanceSingleton<RuntimeDefaultScheduler, RuntimeComponent>(uv);

}, nullptr);

}

}

