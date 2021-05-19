#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/async/taskbasetypes.h>
#include <cold/async/core/coretask.h>
#include <cold/async/core/coretasklinkedlist.h>
#include <cold/async/scheduler.h>
#include <cold/async/asynctimer.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/typeutility.h>
#include <cold/utils/scopeguard.h>
#include <cold/compiler/coroutine.h>

#include <chrono>
#include <initializer_list>
#include <optional>


namespace cold {
namespace async_internal {

RUNTIME_EXPORT void runtimeResumeAfter(std::chrono::milliseconds, async::Scheduler::Ptr, std::coroutine_handle<>) noexcept;



/**
	Rvalue reference task awaiter
*/
template<typename T>
struct RvTaskAwaiter
{
	async::Task<T> task;

	RvTaskAwaiter(async::Task<T>&& task_): task(std::move(task_))
	{}

	bool await_ready() const noexcept
	{
		return task.ready();
	}

	void await_suspend(std::coroutine_handle<> coro) noexcept
	{
		using namespace cold::async;

		core::CoreTask& coreTask = static_cast<core::CoreTaskPtr&>(this->task).coreTask();
		coreTask.setContinuation({Scheduler::Invocation{std::move(coro)}, Scheduler::getCurrent()});
	}

	T await_resume()
	{
		if constexpr (std::is_same_v<void, T>)
		{
			this->task.rethrow();
		}
		else
		{
			return std::move(task).result();
		}
	}
};

/**
	Lvalue reference task awaiter
*/
template<typename T>
struct LvTaskAwaiter
{
	async::Task<T>& task;

	LvTaskAwaiter(async::Task<T>& task_): task(task_)
	{}

	bool await_ready() const noexcept
	{
		return task.ready();
	}

	void await_suspend(std::coroutine_handle<> coro) noexcept
	{
		using namespace cold::async;

		core::CoreTask& coreTask = static_cast<core::CoreTaskPtr&>(this->task).coreTask();
		coreTask.setContinuation({Scheduler::Invocation{std::move(coro)}, Scheduler::getCurrent()});
	}

	T await_resume()
	{
		if constexpr (std::is_same_v<void, T>)
		{
			this->task.rethrow();
		}
		else
		{
			return task.result();
		}
	}
};

/**
*/
struct DelayAwaiter
{
	const std::chrono::milliseconds delay;

	template<typename R, typename P>
	DelayAwaiter(std::chrono::duration<R, P> delay_): delay(std::chrono::duration_cast<std::chrono::milliseconds>(delay_))
	{}

	inline bool await_ready() const noexcept
	{
		return delay == std::chrono::milliseconds(0);
	}

	inline void await_suspend(std::coroutine_handle<> continuation) const noexcept
	{
		runtimeResumeAfter(delay, async::Scheduler::getDefault(), std::move(continuation));
	}

	inline void await_resume() const noexcept
	{}
};


/**

*/
template<typename T>
struct TaskPromiseBase
{
	async::TaskSource<T> taskSource;

	async::Task<T> get_return_object()
	{
		return taskSource.getTask();
	}

	STD_CORO::suspend_never initial_suspend() const noexcept
	{
		return {};
	}

	STD_CORO::suspend_never final_suspend() const noexcept
	{
		return {};
	}

	void unhandled_exception() noexcept
	{
		taskSource.reject(std::current_exception());
	}

	template<typename U>
	static RvTaskAwaiter<U> await_transform(async::Task<U>&& task) noexcept
	{
		return {std::move(task)};
	}

	template<typename U>
	static LvTaskAwaiter<U> await_transform(async::Task<U>& task) noexcept
	{
		return {(task)};
	}

	static async::SchedulerAwaiter await_transform(async::Scheduler::Ptr scheduler) noexcept
	{
		return std::move(scheduler);
	}

	static async::SchedulerAwaiter await_transform(async::Scheduler::WeakPtr scheduler) noexcept
	{
		return scheduler.acquire();
	}

	template<typename Rep, typename Period>
	static DelayAwaiter await_transform(std::chrono::duration<Rep, Period> delay) noexcept
	{
		return delay;
	}
};


RUNTIME_EXPORT bool wait__(async::core::CoreTaskPtr, std::optional<std::chrono::milliseconds>);

RUNTIME_EXPORT async::Task<bool> whenAll__(async::core::TaskContainerIterator, void* iteratorState, std::optional<std::chrono::milliseconds> timeout);

RUNTIME_EXPORT async::Task<bool> whenAny__(async::core::TaskContainerIterator, void* iteratorState, std::optional<std::chrono::milliseconds> timeout);


} // namespace async_internal

namespace async {

template<typename T>
inline constexpr bool IsTask = IsTemplateOf<Task, T>;

/**
*/
template<typename T>
inline bool wait(Task<T>& task, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
{
	return async_internal::wait__(static_cast<core::CoreTaskPtr&>(task), timeout);
}


template<typename T>
inline T waitResult(Task<T> task)
{
	DEBUG_CHECK(task)
	RUNTIME_CHECK(wait(task))

	return std::move(task).result();
}


template<typename T>
inline T waitResult(std::reference_wrapper<Task<T>> taskReference)
{
	decltype(auto) task = static_cast<Task<T>&>(taskReference);
	DEBUG_CHECK(task)
	RUNTIME_CHECK(async::wait(task))

	return task.result();
}

/**
*/
template<typename Container>
inline Task<bool> whenAll(Container& tasks, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
{
	using namespace cold::async_internal;
	using IteratorState = core::TaskContainerIteratorState<Container>;

	IteratorState state{tasks};
	return whenAll__(IteratorState::getIterator(), &state, timeout);
}

/**
*/
template<typename Container>
inline Task<bool> whenAny(Container& tasks, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
{
	using namespace cold::async_internal;
	using IteratorState = core::TaskContainerIteratorState<Container>;

	IteratorState state{tasks};
	return whenAny__(IteratorState::getIterator(), &state, timeout);
}


template<typename T>
inline Task<bool> whenAll(std::initializer_list<Task<T>> tasks, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
{
	using namespace cold::async_internal;
	using IteratorState = core::TaskContainerIteratorState<decltype(tasks)>;

	IteratorState state{tasks};
	return whenAll__(IteratorState::getIterator(), &state, timeout);
}


template<typename T>
inline Task<bool> whenAny(std::initializer_list<Task<T>> tasks, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
{
	using namespace cold::async_internal;
	using IteratorState = core::TaskContainerIteratorState<decltype(tasks)>;

	IteratorState state{tasks};
	return whenAll__(IteratorState::getIterator(), &state, timeout);
}


template<typename F, typename ... Args>
requires (IsTask<std::invoke_result_t<F, Args...>>)
auto run(F operation, Scheduler::Ptr scheduler, Args ... args) -> std::invoke_result_t<F, Args...>
{
	static_assert(std::is_invocable_v<F, Args...>, "Invalid functor. Arguments does not match expected parameters.");

	if (!scheduler)
	{
		scheduler = Scheduler::getDefault();
	}

	if (scheduler.get() != Scheduler::getCurrent().get())
	{
		co_await scheduler;
	}

	using TaskType = std::invoke_result_t<F, Args...>;

	// Scheduler::InvocationGuard invoke {*scheduler}; must be applied above,
	// because currently we are inside of schduler's invocation.
	TaskType task = operation(std::move(args)...);

	//{
	//	Scheduler::InvocationGuard invoke {*scheduler};
	//	task = operation(std::move(args)...);
	//}

	co_return (co_await task);
}


} // namespace async
} // namespace cold



namespace STD_CORO {

template<typename T, typename ... Args>
struct coroutine_traits<cold::async::Task<T>, Args...>
{
	struct promise_type : cold::async_internal::TaskPromiseBase<T>
	{
		template<typename U>
		void return_value(U&& value)
		{
			static_assert(std::is_constructible_v<T, std::decay_t<U>>, "Invalid return value. Check co_return statement.");
			this->taskSource.resolve(std::forward<U>(value));
		}
	};
};


template<typename ... Args>
struct coroutine_traits<cold::async::Task<void>, Args...>
{
	struct promise_type : cold::async_internal::TaskPromiseBase<void>
	{
		void return_void()
		{
			this->taskSource.resolve();
		}
	};
};


} // namespace std
