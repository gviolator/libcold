#pragma once
#include <cold/async/core/coretask.h>
#include <cold/async/task.h>
#include <cold/memory/allocator.h>

#include <atomic>
#include <exception>


namespace cold::async::core {

class CoreTaskImpl final : public CoreTask
{
public:
	CoreTaskImpl(ComPtr<Allocator>, size_t size_, StateDestructorCallback destructor);
	~CoreTaskImpl();

	TaskId id() const override;
	void addRef() override;
	void release() override;
	bool ready() const override;
	std::exception_ptr exception() const override;
	void rethrow() const override;
	const void* data() const override;
	void* data() override;
	size_t dataSize() const override;
	void setContinuation(TaskContinuation) override;
	bool hasContinuation() const override;
	void setReadyCallback(ReadyCallback callback, void*, void* = nullptr) override;
	bool tryResolve__(ResolverCallback, void*) override;

	CoreTaskImpl* next() const;
	void setNext(CoreTaskImpl*);

private:

	void invokeReadyCallback();
	void tryScheduleContinuation();

	ComPtr<Allocator> m_allocator;
	const size_t m_dataSize;
	const StateDestructorCallback m_destructor;
	std::atomic_uint32_t m_refs {1ui32};
	std::atomic_uint32_t m_flags {0ui32};
	std::exception_ptr m_exception;
	TaskContinuation m_continuation;
	Scheduler::CallbackEntry m_readyCallback;
	CoreTaskImpl* m_next = nullptr;
};

} // namespace cold::async::core
