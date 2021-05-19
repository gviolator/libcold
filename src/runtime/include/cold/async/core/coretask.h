#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/async/scheduler.h>
#include <cold/memory/allocator.h>
#include <cold/utils/functor.h>


#include <exception>
#include <memory>
#include <type_traits>


namespace cold::async_internal {

template<typename T>
struct RvTaskAwaiter;

template<typename T>
struct LvTaskAwaiter;

}


namespace cold::async::core {

class CoreTaskPtr;
struct CoreTask;

template<typename T, typename ... Args>
CoreTaskPtr createCoreTask(ComPtr<Allocator>, Args&& ... args);

CoreTask* getCoreTask(CoreTaskPtr&);

/**
*/
struct TaskContinuation
{
	Scheduler::Invocation invocation;
	Scheduler::Ptr scheduler;

	TaskContinuation() = default;
	TaskContinuation(Scheduler::Invocation invocation_, Scheduler::Ptr scheduler_)
		: invocation(std::move(invocation_))
		, scheduler(std::move(scheduler_))
	{}

	explicit operator bool () const
	{
		const bool hasInvocation = std::visit([](auto callable) -> bool {
			return static_cast<bool>(callable);
		}, invocation);

		return hasInvocation;
	}
};


/**
*/
struct CoreTaskOwnership
{
	CoreTask* const ptr;

	CoreTaskOwnership(CoreTask* ptr_): ptr{ptr_}
	{};
	CoreTaskOwnership(const CoreTaskOwnership&) = delete;
};

/**
*/
struct ABSTRACT_TYPE CoreTask
{
	using ReadyCallback = Scheduler::Callback;
	using Rejector = Functor<void (std::exception_ptr) noexcept>;
	using TaskId = uintptr_t;

	virtual TaskId id() const = 0;
	virtual void addRef() = 0;
	virtual void release() = 0;
	virtual bool ready() const = 0;
	virtual std::exception_ptr exception() const = 0;
	virtual void rethrow() const = 0;
	virtual const void* data() const = 0;
	virtual void* data() = 0;
	virtual size_t dataSize() const = 0;
	virtual void setContinuation(TaskContinuation) = 0;
	virtual bool hasContinuation() const = 0;
	virtual void setReadyCallback(ReadyCallback callback, void*, void* = nullptr) = 0;

	bool tryReject(std::exception_ptr exception)
	{
		return this->tryResolve__([](Rejector& rejector, void* ptr) noexcept
		{
			std::exception_ptr* const exception = reinterpret_cast<std::exception_ptr*>(ptr);
			rejector(*exception);
		}, &exception);
	}

	template<typename F>
	requires(std::is_invocable_r_v<void, F, Rejector&>)
	bool tryResolve(F f)
	{
		return this->tryResolve__([](Rejector& rejector, void* ptr) noexcept
		{
			F& callback = *reinterpret_cast<F*>(ptr);
			callback(rejector);
		}, &f);
	}

	bool tryResolve()
	{
		return this->tryResolve__(nullptr, nullptr);
	}

protected:
	using ResolverCallback = void (*)(Rejector&, void*) noexcept;
	using StateDestructorCallback = void (*)(void*) noexcept;

	virtual bool tryResolve__(ResolverCallback, void*) = 0;

private:

	RUNTIME_EXPORT static CoreTaskPtr create(ComPtr<Allocator>, size_t, StateDestructorCallback);

	template<typename T, typename ... Args>
	friend CoreTaskPtr createCoreTask(ComPtr<Allocator>, Args&& ... args);
};



/**
*/
class CoreTaskPtr
{
public:

	virtual ~CoreTaskPtr()
	{
		reset();
	}

	CoreTaskPtr() = default;

	CoreTaskPtr(const CoreTaskPtr& other): m_coreTask(other.m_coreTask)
	{
		if (m_coreTask)
		{
			m_coreTask->addRef();
		}
	}

	CoreTaskPtr(CoreTaskPtr&& other) noexcept : m_coreTask(other.m_coreTask)
	{
		other.m_coreTask = nullptr;
	}

	CoreTaskPtr& operator = (const CoreTaskPtr& other)
	{
		this->reset();
		if (m_coreTask = other.m_coreTask; m_coreTask)
		{
			m_coreTask->addRef();
		}

		return *this;
	}

	CoreTaskPtr& operator = (CoreTaskPtr&& other) noexcept
	{
		this->reset();
		std::swap(m_coreTask, other.m_coreTask);

		return *this;
	}

	explicit operator bool () const noexcept
	{
		return m_coreTask != nullptr;
	}

protected:

	CoreTaskPtr(const CoreTaskOwnership& ownership_): m_coreTask(ownership_.ptr)
	{}

	CoreTask& coreTask()
	{
		DEBUG_CHECK(static_cast<bool>(*this), "Task is stateless")
		return *m_coreTask;
	}

	const CoreTask& coreTask() const
	{
		DEBUG_CHECK(static_cast<bool>(*this), "Task is stateless")
		return *m_coreTask;
	}

	void reset()
	{
		if (m_coreTask)
		{
			m_coreTask->release();
		}

		m_coreTask = nullptr;
	}

private:

	CoreTask* m_coreTask = nullptr;

	friend struct CoreTask;

	template<typename T, typename ... Args>
	friend CoreTaskPtr createCoreTask(ComPtr<Allocator>, Args&& ... args);

	friend CoreTask* getCoreTask(CoreTaskPtr&);

	template<typename T>
	friend struct async_internal::RvTaskAwaiter;

	template<typename T>
	friend struct async_internal::LvTaskAwaiter;
};


template<typename T, typename ... Args>
CoreTaskPtr createCoreTask(ComPtr<Allocator> allocator, Args&& ... args)
{

	CoreTaskPtr coreTaskPtr = CoreTask::create(std::move(allocator), sizeof(T), [](void* ptr) noexcept
	{
		T* const state = reinterpret_cast<T*>(ptr);
		std::destroy_at(state);
	});

	new (coreTaskPtr.coreTask().data()) T(std::forward<Args>(args)...);

	return coreTaskPtr;
}


inline CoreTask* getCoreTask(CoreTaskPtr& coreTaskPtr)
{
	return coreTaskPtr ? &coreTaskPtr.coreTask() : nullptr;
}

}
