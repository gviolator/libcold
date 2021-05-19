#include "pch.h"

#include "cold/async/scheduler.h"
#include "cold/async/core/schedulerguard.h"
#include "cold/com/weakcomptr.h"

//#include "threading/event.h"
//#include "utils/dllreference.h"


namespace cold::async {

namespace {

/**
* 
*/
Scheduler* currentThreadInvokedScheduler(std::optional<Scheduler*> scheduler = std::nullopt) noexcept
{
	static thread_local Scheduler* invokedScheduler__ = nullptr;

	if (scheduler)
	{
		Scheduler* const temp = invokedScheduler__;
		invokedScheduler__ = *scheduler;
		return temp;
	}

	return invokedScheduler__;
}


Scheduler::Ptr threadDefaultScheduler(std::optional<Scheduler::Ptr> scheduler = std::nullopt) noexcept
{
	static thread_local WeakComPtr<Scheduler> threadScheduler__;

	if (scheduler)
	{
		DEBUG_CHECK(!*scheduler || threadScheduler__.isDead(), "Thread (%1) already has assigned scheduler", std::this_thread::get_id())

		threadScheduler__ = std::move(*scheduler);
	}

	return threadScheduler__.acquire();
}


Scheduler::Ptr defaultScheduler(std::optional<Scheduler::Ptr> scheduler = std::nullopt) noexcept
{
	static WeakComPtr<Scheduler> defaultScheduler__;

	if (scheduler)
	{
		defaultScheduler__ = std::move(*scheduler);
	}

	return defaultScheduler__.acquire();
}

} // namespace

//-----------------------------------------------------------------------------
Scheduler::InvocationGuard::InvocationGuard(Scheduler& scheduler): threadId(GetCurrentThreadId())
{
	RUNTIME_CHECK(currentThreadInvokedScheduler(&scheduler) == nullptr, "Thread already has invoked scheduler")
}


Scheduler::InvocationGuard::~InvocationGuard()
{
	DEBUG_CHECK(this->threadId == static_cast<uint64_t>(GetCurrentThreadId()), "Invalid thread: (%1), scheduler invocation thread: (%2)", GetCurrentThreadId(), this->threadId)
	RUNTIME_CHECK(currentThreadInvokedScheduler(nullptr) != nullptr)
}

//-----------------------------------------------------------------------------
void Scheduler::invoke(Scheduler& scheduler, Invocation invocation) noexcept
{
	DEBUG_CHECK(currentThreadInvokedScheduler() != nullptr, "Invoked scheduler not set. Use Scheduler::InvocationGuard.")
	DEBUG_CHECK(currentThreadInvokedScheduler() == &scheduler, "Invalid scheduler: current scheduler mismatch.")

	std::visit([](auto& callable)
	{
		DEBUG_CHECK(callable)

		using CallableType = std::remove_reference_t<decltype(callable)>;

		static_assert(
			std::is_same_v<CallableType, std::coroutine_handle<>> ||
			std::is_same_v<CallableType, CallbackEntry>,
			"Unknown callable type");

		callable();

	}, invocation);
}


void Scheduler::schedule(Invocation invocation) noexcept
{
	scheduleInvocation(std::move(invocation));
}


void Scheduler::schedule(std::coroutine_handle<> coro) noexcept
{
	schedule(Invocation{std::in_place_type<std::coroutine_handle<>>, std::move(coro)});
}


void Scheduler::schedule(Scheduler::Callback callback, void* data1, void* data2) noexcept
{
	schedule(Invocation{std::in_place_type<Scheduler::CallbackEntry>, callback, data1, data2});
}


void Scheduler::finalize(Scheduler::Ptr&& scheduler_)
{
	using namespace std::chrono;

	Scheduler::Ptr scheduler = std::move(scheduler_);

	auto checkFinalizeTooLong = [time = system_clock::now(), notified = false]() mutable
	{
		constexpr milliseconds ShutdownTimeout {5s};
		
		if (system_clock::now() - time >= ShutdownTimeout && !notified)
		{
			notified = true;
			LOG_debug_("Scheduler going shutting down too long")
			//core::dumpActiveTasks();
		}
	};

	do
	{
		scheduler->waitAnyActivity();
		if (const bool noReferences = scheduler->refsCount() == 1; noReferences)
		{
			break;
		}

		checkFinalizeTooLong();
	}
	while (true);
}


Scheduler::Ptr Scheduler::getDefault()
{
	return async::defaultScheduler();
}


Scheduler::Ptr Scheduler::getCurrent()
{
	if (auto scheduler = Scheduler::getInvoked(); scheduler)
	{
		return scheduler;
	}

	if (auto scheduler = Scheduler::currentThreadScheduler(); scheduler)
	{
		return scheduler;
	}

	return getDefault();
}


Scheduler::Ptr Scheduler::getInvoked()
{
	if (Scheduler* const scheduler = async::currentThreadInvokedScheduler(); scheduler)
	{
		return com::Acquire {scheduler};
	}

	return {};
}


Scheduler::Ptr Scheduler::currentThreadScheduler()
{
	return async::threadDefaultScheduler();
}


void Scheduler::setDefault(Scheduler::Ptr scheduler)
{
	async::defaultScheduler(std::move(scheduler));
}


void Scheduler::setCurrentThreadScheduler(Scheduler::Ptr scheduler)
{
	async::threadDefaultScheduler(std::move(scheduler));
}


} // namespace cold::async

//-------------------------------------------------------
#if 0

using namespace std::chrono;

namespace cold::async {


namespace core {

void dumpActiveTasks();


namespace {


/// <summary>
///
/// </summary>
/// <param name="changeScheduler"></param>
/// <returns></returns>
Scheduler* currentThreadInvokedScheduler(std::optional<Scheduler*> scheduler = std::nullopt) noexcept
{
	static thread_local std::list<Scheduler*> schedulers;

	if (scheduler)
	{
		if (*scheduler == nullptr)
		{
			DEBUG_CHECK(!schedulers.empty())

			auto top = schedulers.begin();

			schedulers.erase(top);
		}
		else
		{
			schedulers.push_front(*scheduler);
		}
	}

	return schedulers.empty() ? nullptr : schedulers.front();
}


/// <summary>
///
/// </summary>
/// <param name="scheduler"></param>
/// <returns></returns>
SchedulerPtr threadDefaultScheduler(SchedulerPtr scheduler = SchedulerPtr{}) noexcept
{
	static thread_local std::weak_ptr<Scheduler> currentThreadScheduler;

	if (scheduler)
	{
		DEBUG_CHECK(currentThreadScheduler.expired(), "Thread (%1) already has assigned scheduler", std::this_thread::get_id())

		currentThreadScheduler = std::move(scheduler);
	}

	return currentThreadScheduler.lock();

}

/// <summary>
///
/// </summary>
/// <returns></returns>
SchedulerPtr internalDefaultScheduler()
{
	static SchedulerGuard s_scheduler = createThreadpoolScheduler();

	return Scheduler::find(s_scheduler->uid());
}


/// <summary>
///
/// </summary>
/// <param name="schedulerToSet"></param>
/// <returns></returns>
SchedulerPtr defaultScheduler(std::optional<SchedulerPtr> schedulerToSet = std::nullopt) noexcept
{
	static Uid s_defaultSchedulerUid ;

	if (schedulerToSet)
	{
		SchedulerPtr scheduler = *schedulerToSet;

		s_defaultSchedulerUid = scheduler ? scheduler->uid() : Uid{};
	}

	SchedulerPtr scheduler;

	if (s_defaultSchedulerUid)
	{
		scheduler = Scheduler::find(s_defaultSchedulerUid);
	}

	return scheduler ? scheduler : internalDefaultScheduler();
}



/// <summary>
///
/// </summary>
class SchedulerManager
{

public:

	static SchedulerManager& instance()
	{
		static SchedulerManager s_manager;

		return s_manager;
	}


	Uid add(SchedulerPtr scheduler)
	{
		std::lock_guard<std::recursive_mutex> lock(m_schedulersMutex);

		auto uid = Uid::generate();

		m_schedulers.emplace(uid, std::move(scheduler));

		return uid;
	}


	void bindSchedulerName(const Scheduler& scheduler, std::string name)
	{
		std::lock_guard<std::recursive_mutex> lock(m_schedulersMutex);

		SchedulerManager::SchedulerEntry* entry = nullptr;

		for (auto& [uid, schedulerEntry] : m_schedulers)
		{
			CHECK(schedulerEntry.name.empty() || (schedulerEntry.name != name));

			if (schedulerEntry.scheduler.get() == &scheduler)
			{
				entry = &schedulerEntry;
			}
		}

		DEBUG_CHECK(entry != nullptr);

		entry->name = std::move(name);
	}


	bool remove(Uid schedulerUid)
	{
		std::lock_guard<std::recursive_mutex> lock(m_schedulersMutex);

		auto iter = m_schedulers.find(schedulerUid);

		if (iter == m_schedulers.end())
		{
			return false;
		}

		if (iter->second.scheduler.use_count() == 1)
		{
			m_schedulers.erase(iter);

			return true;
		}

		return false;
	}


	SchedulerPtr find(Uid schedulerUid) const
	{
		std::lock_guard<std::recursive_mutex> lock(m_schedulersMutex);

		auto iter = m_schedulers.find(schedulerUid);

		return iter != m_schedulers.end() ? iter->second.scheduler : SchedulerPtr{};
	}


	SchedulerPtr findByName(std::string_view name) const
	{
		if (name.empty())
		{
			return {};
		}

		std::lock_guard<std::recursive_mutex> lock(m_schedulersMutex);

		for (auto& [uid, schedulerEntry] : m_schedulers)
		{
			if (schedulerEntry.name == name)
			{
				return schedulerEntry.scheduler;
			}
		}

		return {};
	}


	Uid schedulerUid(const Scheduler& scheduler) const
	{
		std::lock_guard<std::recursive_mutex> lock(m_schedulersMutex);

		for (auto& [uid, schedulerEntry] : m_schedulers)
		{
			if (schedulerEntry.scheduler.get() == &scheduler)
			{
				return uid;
			}
		}

		CHECK_FAILURE("Scheduler not found")

		return Uid{};
	}


private:

	struct SchedulerEntry
	{
		SchedulerPtr scheduler;

		std::string name;

		SchedulerEntry(SchedulerPtr scheduler_)
			: scheduler(std::move(scheduler_))
		{}
	};


	mutable std::recursive_mutex m_schedulersMutex;

	std::map<Uid, SchedulerEntry> m_schedulers;
};


} // unnamed namespace

} // namespace core


namespace async_internal {

/// <summary>
///
/// </summary>
class SchedulerGuardImpl
	: public core::ISchedulerGuard
{
	struct DelayNotificationGuard
	{
		static constexpr milliseconds ShutdownTimeout {5s};

		system_clock::time_point time = system_clock::now();

		bool delayNotified = false;

		void check()
		{
			if (system_clock::now() - time >= ShutdownTimeout && !delayNotified)
			{
				delayNotified = true;

				LOG_Debug("Scheduler going shutting down too long")

				core::dumpActiveTasks();
			}
		}

	};

public:

	SchedulerGuardImpl(SchedulerPtr scheduler, HMODULE dll)
		: m_schedulerUid(core::SchedulerManager::instance().add(std::move(scheduler)))
		, m_dll(dll)
	{
	}

	~SchedulerGuardImpl()
	{
		DelayNotificationGuard delayNotification;

		do
		{
			{// Must do not keep scheduler ptr ownership here, because in this case Scheduler manager never remove it (because it require no external references to the scheduler instance).
				auto scheduler = checkNotNull(core::SchedulerManager::instance().find(m_schedulerUid));

				scheduler->waitAnyActivity();
			}

			delayNotification.check();
		}
		while (!core::SchedulerManager::instance().remove(m_schedulerUid));
	}

private:

	virtual Uid uid() const noexcept override
	{
		return m_schedulerUid;
	}


	Uid m_schedulerUid;

	DllReference m_dll;
};


//-------------------------------------------------------------------------------------------------
std::unique_ptr<core::ISchedulerGuard> createSchedulerGuard(core::SchedulerPtr scheduler, HMODULE dll)
{
	return std::make_unique<SchedulerGuardImpl>(std::move(scheduler), dll);
}


} // namespace async_internal


namespace core {


/// <summary>
///
/// </summary>
Scheduler::InvocationGuard::InvocationGuard(Scheduler& scheduler)
	: threadId(static_cast<uint64_t>(GetCurrentThreadId()))
{
	currentThreadInvokedScheduler(&scheduler);
}


Scheduler::InvocationGuard::~InvocationGuard()
{
	DEBUG_CHECK(threadId == static_cast<uint64_t>(GetCurrentThreadId()), "Invalid thread. Possible Scheduler::InvocationGuard used in scope of async operation")

	currentThreadInvokedScheduler(std::optional<Scheduler*>{nullptr});
}


//-----------------------------------------------------------------------------
void Scheduler::schedule(Invocation invocation) noexcept
{
	//if (canInvokeInplace())
	//{
	//	InvocationGuard invokeGuard(*this);

	//	invoke(*this, std::move(invocation));

	//	return;
	//}

	scheduleInvocation(std::move(invocation));
}


void Scheduler::schedule(std::unique_ptr<TaskContinuation> continuation) noexcept
{
	schedule(makeInvocation(std::move(continuation)));
}


void Scheduler::schedule(std::coroutine_handle<> coro) noexcept
{
	schedule(makeInvocation(std::move(coro)));
}


void Scheduler::schedule(Scheduler::Callback callback, void* callbackData) noexcept
{
	schedule(makeInvocation(callback, callbackData));
}


Uid Scheduler::uid() const
{
	return SchedulerManager::instance().schedulerUid(*this);
}


bool Scheduler::canInvokeInplace() const noexcept
{
	return false;
}


void Scheduler::invoke([[maybe_unused]]Scheduler& scheduler, Invocation invocation) noexcept
{
	DEBUG_CHECK(currentThreadInvokedScheduler() != nullptr, "Invoked scheduler not set. Use Scheduler::InvocationGuard.")
	DEBUG_CHECK(currentThreadInvokedScheduler() == &scheduler, "Invalid scheduler: current scheduler mismatch.")


	std::visit([](auto& callable)
	{
		DEBUG_CHECK(callable)

		using CallableType = std::remove_reference_t<decltype(callable)>;

		if constexpr (std::is_same_v<CallableType, std::coroutine_handle<>>)
		{
			callable();
		}
		else if constexpr (std::is_same_v<CallableType, std::unique_ptr<TaskContinuation>>)
		{
			callable->invoke();
		}
		else if constexpr (std::is_same_v<CallableType, CallbackEntry>)
		{
			callable();
		}
		else
		{
			static_assert(false, "Unknown callable type");
		}
	}, invocation);
}


SchedulerPtr Scheduler::getCurrent()
{
	if (auto scheduler = getInvoked(); scheduler)
	{
		return scheduler;
	}

	if (auto scheduler = threadDefaultScheduler(); scheduler)
	{
		return scheduler;
	}

	return getDefault();
}


SchedulerPtr Scheduler::getDefault()
{
	return async::core::defaultScheduler();
}


SchedulerPtr Scheduler::getInvoked()
{
	if (auto scheduler = currentThreadInvokedScheduler(); scheduler)
	{
		return scheduler->asShared<Scheduler>();
	}

	return {};
}


void Scheduler::setDefault(SchedulerPtr scheduler)
{
	async::core::defaultScheduler(std::move(scheduler));
}


SchedulerPtr Scheduler::find(Uid schedulerUid)
{
	return SchedulerManager::instance().find(schedulerUid);
}


SchedulerPtr Scheduler::findByName(std::string_view name)
{
	return SchedulerManager::instance().findByName(name);
}


void Scheduler::setCurrentThreadScheduler(SchedulerPtr scheduler)
{
	threadDefaultScheduler(std::move(scheduler));
}


SchedulerPtr Scheduler::currentThreadScheduler()
{
	return threadDefaultScheduler();
}


void Scheduler::bindSchedulerName(SchedulerPtr scheduler, std::string_view name)
{
	DEBUG_CHECK(scheduler)

	return SchedulerManager::instance().bindSchedulerName(*scheduler, std::string{name});
}



} // namespace core

} // naemspace cold::async


namespace std {

bool await_ready(const cold::async::SchedulerPtr& scheduler) noexcept
{
	DEBUG_CHECK(scheduler)

	return scheduler->canInvokeInplace();
}


void await_suspend(cold::async::SchedulerPtr& scheduler, std::coroutine_handle<> coro) noexcept
{
	DEBUG_CHECK(scheduler)

	scheduler->schedule(std::move(coro));
}


void await_resume([[maybe_unused]] const cold::async::SchedulerPtr& scheduler) noexcept
{
	DEBUG_CHECK(scheduler)
}

} // namespace std


#endif