#pragma once

#include <cold/async/scheduler.h>
#include <cold/async/core/coretask.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/scopeguard.h>
#include <cold/utils/nothing.h>

#include <exception>
#include <type_traits>

namespace cold::async {

template<typename T>
class TaskBase;

template<typename T>
class TaskSourceBase;

template<typename T = void>
class Task;

template<typename T = void>
class TaskSource;

} // namespace cold::async


namespace cold::async_internal {

template<typename T>
struct TaskData;

template<>
struct TaskData<void>
{
	bool taskDetached = false;
	bool taskGivenOut = false;
};

template<typename T>
struct TaskData : TaskData<void>
{
	std::optional<T> result;
};


template<typename T>
class TaskStateHolder : public async::core::CoreTaskPtr
{
public:
	TaskStateHolder(const TaskStateHolder<T>&) = delete;
	TaskStateHolder(TaskStateHolder<T>&&) noexcept = default;
	TaskStateHolder& operator = (const TaskStateHolder&) = delete;
	TaskStateHolder& operator = (TaskStateHolder&&) = default;


	bool ready() const
	{
		DEBUG_CHECK(static_cast<bool>(*this), "Task is stateless")
		return this->coreTask().ready();
	}

protected:

	TaskStateHolder() = default;

	TaskStateHolder(async::core::CoreTaskPtr&& coreTask_): async::core::CoreTaskPtr(std::move(coreTask_))
	{}

	const TaskData<T>& taskData() const &
	{
		const void* const ptr = this->coreTask().data();
		return *reinterpret_cast<const TaskData<T>*>(ptr);
	}

	TaskData<T>& taskData() &
	{
		void* const ptr = this->coreTask().data();
		return *reinterpret_cast<TaskData<T>*>(ptr);
	}

	TaskData<T>&& taskData() &&
	{
		void* const ptr = this->coreTask().data();
		return std::move(*reinterpret_cast<TaskData<T>*>(ptr));
	}
};


} // namespace cold::async_internal


namespace cold {
namespace async {

#pragma region Task

/**
*/
template<typename T>
class TaskBase : public async_internal::TaskStateHolder<T>
{
public:
	using ValueType = T;

	static Task<T> makeRejected(std::exception_ptr exception) noexcept;

	template<typename E>
	requires (std::is_base_of_v<std::exception, E>)
	static Task<T> makeRejected(E exception) noexcept;


	TaskBase() = default;
	TaskBase(const TaskBase<T>&) = delete;
	TaskBase(TaskBase<T>&&) noexcept = default;
	TaskBase<T>& operator = (const TaskBase<T>&) = delete;
	TaskBase<T>& operator = (TaskBase<T>&&) = default;


	void rethrow() const
	{
		this->coreTask().rethrow();
	}

	std::exception_ptr exception() const
	{
		return this->coreTask().exception();
	}

	bool rejected() const
	{
		return static_cast<bool>(this->coreTask().exception());
	}
	
	Task<T>& detach() &
	{
		DEBUG_CHECK(!this->taskData().taskDetached, "Task already detached")
		this->taskData().taskDetached = true;

		return static_cast<Task<T>&>(*this);
	}

	Task<T>&& detach() &&
	{
		DEBUG_CHECK(!this->taskData().taskDetached, "Task already detached")
		this->taskData().taskDetached = true;

		return std::move(static_cast<Task<T>&>(*this));
	}

protected:

	TaskBase(async::core::CoreTaskPtr&& coreTask_): async_internal::TaskStateHolder<T>(std::move(coreTask_))
	{}

	~TaskBase()
	{
		if (std::uncaught_exceptions() > 0 || !static_cast<bool>(*this))
		{
			return;
		}

#if !defined(NDEBUG)
		[[maybe_unused]] const bool taskReadyOrDetached = this->ready() || this->taskData().taskDetached || this->coreTask().hasContinuation();
		DEBUG_CHECK(taskReadyOrDetached, "Not finished async::Task<> is leaving its scope. Use co_await or set continuation.")
#endif
	}
};

/**
*/
template<typename T>
class [[nodiscard]] Task final : public TaskBase<T>
{
public:
	template<typename ... Args>
	static Task<T> makeResolved(Args&&...)  requires(std::is_constructible_v<T, Args...>);

	Task() = default;
	Task(const Task<T>&) = delete;
	Task(Task<T>&&) = default;
	Task<T>& operator = (const Task<T>&) = delete;
	Task<T>& operator = (Task<T>&&) = default;


	T result() & requires(std::is_copy_assignable_v<T>)
	{
		DEBUG_CHECK(this->ready(), "Task<{0}> is not ready", typeid(T))
		this->rethrow();

		DEBUG_CHECK(this->taskData().result)
		return *this->taskData().result;
	}

	T result() &&
	{
		DEBUG_CHECK(this->ready(), "Task<{0}> is not ready", typeid(T))

		SCOPE_Leave {
			this->reset();
		};

		this->rethrow();
		DEBUG_CHECK(this->taskData().result)
		return std::move(*this->taskData().result);
	}

	T operator * () &
	{
		return this->result();
	}

	T operator * () &&
	{
		return this->result();
	}

private:
	Task(async::core::CoreTaskPtr&& coreTask_): TaskBase<T>(std::move(coreTask_))
	{}

	friend class TaskSourceBase<T>;
};

/**
*/
template<>
class [[nodiscard]] Task<void> final : public TaskBase<void>
{
public:
	static Task<void> makeResolved();

	Task() = default;
	Task(const Task<>&) = delete;
	Task(Task<>&&) = default;
	Task<>& operator = (const Task<>&) = delete;
	Task<>& operator = (Task<>&&) = default;


	void result() const &
	{
		this->rethrow();
	}

	void result() &&
	{
		SCOPE_Leave {
			this->reset();
		};

		this->rethrow();
	}

private:
	Task(async::core::CoreTaskPtr&& coreTask_): TaskBase<void>(std::move(coreTask_))
	{}

	friend class TaskSourceBase<void>;
};

#pragma endregion


#pragma region TaskSource

/**
*/
template<typename T>
class TaskSourceBase : public async_internal::TaskStateHolder<T>
{
public:

	TaskSourceBase(Nothing) {}
	TaskSourceBase(TaskSourceBase<T>&&) = default;
	TaskSourceBase(const TaskSourceBase<T>&) = delete;
	TaskSourceBase& operator = (const TaskSourceBase<T>&) = delete;
	TaskSourceBase& operator = (TaskSourceBase<T>&&) = default;

	bool reject(std::exception_ptr exception) noexcept
	{
		return this->coreTask().tryReject(std::move(exception));
	}

	template<typename E>
	requires (std::is_base_of_v<std::exception, E>)
	bool reject(E exception) noexcept
	{
		return this->coreTask().tryReject(std::make_exception_ptr(std::move(exception)));
	}

	Task<T> getTask()
	{
		DEBUG_CHECK(!this->taskData().taskGivenOut, "Task<{0}> already takedout from source", typeid(T))

		this->taskData().taskGivenOut = true;
		async::core::CoreTaskPtr coreTask = static_cast<async::core::CoreTaskPtr&>(*this);
		return Task<T>{std::move(coreTask)};
	}

protected:

	TaskSourceBase()
		: async_internal::TaskStateHolder<T>{core::createCoreTask<async_internal::TaskData<T>>(ComPtr<Allocator>{})}
	{}

	virtual ~TaskSourceBase()
	{
		if (static_cast<bool>(*this) && !this->ready())
		{
			this->reject(std::exception("Task source destructed, but result is not set"));
		}
	}
};

/**
*/
template<typename T>
class TaskSource final : public TaskSourceBase<T>
{
public:
	using TaskSourceBase<T>::TaskSourceBase;

	template<typename ... Args>
	bool resolve(Args&& ... args) requires(std::is_constructible_v<T, Args...>)
	{
		return this->coreTask().tryResolve([&](core::CoreTask::Rejector&) noexcept
		{
			this->taskData().result.emplace(std::forward<Args>(args)...);
		});
	}
};

/**
*/
template<>
class TaskSource<void> final : public TaskSourceBase<void>
{
public:
	using TaskSourceBase<void>::TaskSourceBase;

	bool resolve()
	{
		return this->coreTask().tryResolve();
	}
};

#pragma endregion


template<typename T>
Task<T> TaskBase<T>::makeRejected(std::exception_ptr exception) noexcept
{
	TaskSource<T> taskSource;
	taskSource.setException(std::move(exception));
	return taskSource.getTask();
}

template<typename T>
template<typename E>
requires (std::is_base_of_v<std::exception, E>)
Task<T> TaskBase<T>::makeRejected(E exception) noexcept
{
	TaskSource<T> taskSource;
	taskSource.setException(std::move(exception));
	return taskSource.getTask();
}

template<typename T>
template<typename ... Args>
Task<T> Task<T>::makeResolved(Args&&...args) requires(std::is_constructible_v<T, Args...>)
{
	TaskSource<T> taskSource;
	taskSource.resolve(std::forward<Args>(args)...);
	return taskSource.getTask();
}

inline Task<void> Task<void>::makeResolved()
{
	TaskSource<> taskSource;
	taskSource.resolve();
	return taskSource.getTask();
}


} // namespace async
} // namespace cold
