#include "pch.h"
#include "cold/async/asynctimer.h"
#include "cold/async/task.h"

#include "cold/runtime/runtime.h"
#include "cold/runtime/internal/runtimeinternal.h"
#include "cold/runtime/internal/runtimeallocator.h"
#include "cold/runtime/internal/uvutils.h"
#include "cold/runtime/internal/runtimecomponent.h"

#include "cold/diagnostics/runtimecheck.h"
#include "cold/utils/scopeguard.h"
#include "cold/threading/critical_section.h"
#include "cold/threading/event.h"
#include "cold/com/comclass.h"


using namespace std::chrono;

namespace cold {
namespace async {

namespace {

constexpr std::string_view TimerManagerComponentId = "timer_manager";


struct ResumeAfterData
{
	const Scheduler::Ptr scheduler;
	const std::coroutine_handle<> continuation;
	TaskSource<> awaiterSource;

	ResumeAfterData() = default;

	ResumeAfterData(Scheduler::Ptr&& scheduler_, std::coroutine_handle<> continuation_)
		: scheduler(std::move(scheduler_))
		, continuation(std::move(continuation_))
	{}

	void schedule()
	{
		DEBUG_CHECK(scheduler)
		DEBUG_CHECK(continuation)

		scheduler->schedule(continuation);

		awaiterSource.resolve();
	}

	Task<> getAwaiter()
	{
		return awaiterSource.getTask();
	}
};

} // namespace



/**
*/
class TimerManagerImpl : public RuntimeComponent, public Disposable
{
	COMCLASS_(RuntimeComponent, Disposable)

public:

	static TimerManagerImpl& getThis()
	{
		ComPtr<> component = Runtime::instance().findComponent(TimerManagerComponentId);
		DEBUG_CHECK(component)

		return component->as<TimerManagerImpl&>();
	}

	TimerManagerImpl(uv_loop_t* uv): m_uv(uv)
	{}

	~TimerManagerImpl()
	{
		DEBUG_CHECK(Runtime::isRuntimeThread())
		DEBUG_CHECK(m_timers.empty())
	}

	Task<> resumeAfter(milliseconds delay, Scheduler::Ptr scheduler, std::coroutine_handle<> continuation)
	{
		KeepRC(*this);
		SWITCH_RUNTIME_SCHEDULER

		ResumeAfterData data{std::move(scheduler), std::move(continuation)};
		uv_timer_t* const timer = initializeTimerHandle();
		timer->data = &data;

		const auto callback = [](uv_timer_t* timer) noexcept
		{
			SCOPE_Leave {
				TimerManagerImpl::getThis().releaseTimerHandle(timer);
			};

			reinterpret_cast<ResumeAfterData*>(timer->data)->schedule();
		};

		UV_RUNTIME_CHECK(uv_timer_start(timer, callback, static_cast<uintptr_t>(delay.count()), 0))

		co_await data.getAwaiter();
	}

	uv_timer_t* initializeTimerHandle()
	{
		DEBUG_CHECK(Runtime::isRuntimeThread())

		uv_timer_t* const timer = &m_timers.emplace_back();
		uv_timer_init(m_uv, timer);
		return timer;
	}

	void releaseTimerHandle(uv_timer_t* timer)
	{
		DEBUG_CHECK(Runtime::isRuntimeThread())
		DEBUG_CHECK(timer)

		uv_handle_t* const handle = asUvHandle(timer);
		DEBUG_CHECK(uv_is_closing(handle) == 0)

		uv_close(handle, [](uv_handle_t* handle) noexcept
		{ // timer manager owns the handle, so it must be released only after UV will close it.
			TimerManagerImpl& this_ = getThis();
			uv_timer_t* const timer = reinterpret_cast<uv_timer_t*>(handle);
			auto iter = std::find_if(this_.m_timers.begin(), this_.m_timers.end(), [timer](const uv_timer_t& otherTimer){ return timer == &otherTimer;});
			DEBUG_CHECK(iter != this_.m_timers.end())

			this_.m_timers.erase(iter);
		});
	}

private:

	void dispose() override
	{
		DEBUG_CHECK(Runtime::isRuntimeThread())
		for (uv_timer_t& timer : m_timers)
		{
			releaseTimerHandle(&timer);
		}
	}

	bool componentHasWorks() override
	{
		return !m_timers.empty();
	}

	uv_loop_t* const m_uv;
	std::pmr::list<uv_timer_t> m_timers{runtimePoolMemoryResource()};
};

/**
*/
class AsyncTimerImpl final : public AsyncTimer
{
	COMCLASS_(AsyncTimer)

public:

	AsyncTimerImpl(milliseconds dueTime_, std::optional<milliseconds> repeat_)
		: m_dueTime(dueTime_)
		, m_repeat(repeat_ ? *repeat_ : 0ms)
	{
		m_tickSource.emplace();

		[](AsyncTimerImpl& this_) -> Task<>
		{
			KeepRC(this_);
			SWITCH_RUNTIME_SCHEDULER

			if (this_.refsCount() > 1) // If there is no external references, do not need to start timer at all.
			{
				this_.startTimer();
			}

		}(*this).detach();
	}


	~AsyncTimerImpl()
	{
		killTimer();
	}

	Task<bool> tick() override
	{
		const std::lock_guard lock{m_mutex};

		if (!m_tickSource)
		{
			return Task<bool>::makeResolved(false);
		}

		DEBUG_CHECK(*m_tickSource, "Invalid timer state")
		return m_tickSource->getTask();
	}

	void stop() override
	{
		killTimer();
	}

private:

	void startTimer()
	{
		DEBUG_CHECK(Runtime::isRuntimeThread())

		TimerManagerImpl& timerManager = TimerManagerImpl::getThis();
		m_timer = timerManager.initializeTimerHandle();
		m_timer->data = this;

		const uint64_t timeout = static_cast<uint64_t>(m_dueTime.count());
		const uint64_t repeat = static_cast<uint64_t>(m_repeat.count());

		auto callback = [](uv_timer_t* timer) noexcept
		{
			AsyncTimerImpl& this_ = *reinterpret_cast<AsyncTimerImpl*>(timer->data);

			if (this_.m_repeat == 0ms)
			{
				this_.releaseTimer();
			}
			else
			{
				std::lock_guard lock{this_.m_mutex};
				this_.m_tickSource->resolve(true);
				this_.m_tickSource.emplace();
			}
		};

		UV_RUNTIME_CHECK(uv_timer_start(m_timer, callback, timeout, repeat))
	}

	void killTimer()
	{
		if (const std::lock_guard lock{m_mutex}; !m_timer)
		{
			return;
		}

		if (Runtime::isRuntimeThread())
		{
			releaseTimer();
			return ;
		}

		threading::Event signal;

		RuntimeInternal::scheduler()->schedule([](void* thisPtr, void* signalPtr) noexcept
		{
			AsyncTimerImpl& this_ = *reinterpret_cast<AsyncTimerImpl*>(thisPtr);
			this_.releaseTimer();

			reinterpret_cast<threading::Event*>(signalPtr)->set();

		}, this, &signal);

		signal.wait();
	}

	void releaseTimer()
	{
		DEBUG_CHECK(Runtime::isRuntimeThread())

		const std::lock_guard lock{m_mutex};
		if (m_tickSource)
		{
			DEBUG_CHECK(*m_tickSource, "Invalid timer state while killTimer")
			m_tickSource->resolve(false);
			m_tickSource = std::nullopt;
		}

		TimerManagerImpl::getThis().releaseTimerHandle(m_timer);
		m_timer = nullptr;
	}


	const milliseconds m_dueTime;
	const milliseconds m_repeat;
	uv_timer_t* m_timer = nullptr;
	std::optional<TaskSource<bool>> m_tickSource;
	threading::CriticalSection m_mutex;
};


//-----------------------------------------------------------------------------
ComPtr<AsyncTimer> createRuntimeTimer(milliseconds dueTime, std::optional<milliseconds> repeat, ComPtr<Allocator> allocator)
{
	return com::createInstanceWithAllocator<AsyncTimerImpl, AsyncTimer>(allocator ? std::move(allocator) : crtAllocator(), dueTime, repeat);
}


namespace {

[[maybe_unused]] const auto registration = RuntimeComponent::registerComponent(TimerManagerComponentId, [](uv_loop_t* uv, std::string_view, void*) noexcept -> RuntimeComponent::Ptr
{
	return com::createInstanceSingleton<TimerManagerImpl, RuntimeComponent>(uv);

}, nullptr);

} // namespace
} // namespace async


//-----------------------------------------------------------------------------
namespace async_internal {

void runtimeResumeAfter(milliseconds delay, async::Scheduler::Ptr scheduler, std::coroutine_handle<> continuation) noexcept
{
	using namespace async;

	auto& manager = TimerManagerImpl::getThis();
	manager.resumeAfter(delay, std::move(scheduler), continuation).detach();
}

} // namespace async_internal
} // namespace cold


