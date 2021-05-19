#include "pch.h"
#include "coretaskimpl.h"
#include "cold/diagnostics/runtimecheck.h"
#include "cold/memory/membase.h"
#include "cold/utils/scopeguard.h"

#include <memory>
#include <type_traits>


namespace cold::async::core {

//-----------------------------------------------------------------------------
namespace {

constexpr size_t DefaultAlign = sizeof(std::max_align_t);
constexpr size_t CoreTaskSize = sizeof(std::aligned_storage_t<sizeof(CoreTaskImpl)>);

constexpr static uint32_t TaskFlag_Ready = 1ui32 << 0;
constexpr static uint32_t TaskFlag_HasContinuation = 1ui32 << 2;
constexpr static uint32_t TaskFlag_ContinuationScheduled = 1ui32 << 3;
constexpr static uint32_t TaskFlag_ResolveLocked = 1ui32 << 5;
constexpr static uint32_t TaskFlag_ReadyCallbackLocked = 1ui32 << 6;

inline bool hasFlags(const std::atomic_uint32_t& bits, uint32_t mask)
{
	return (bits.load() & mask) == mask;
}

inline void setFlagsOnce(std::atomic_uint32_t& bits, uint32_t mask)
{
	uint32_t value = bits.load();

	do
	{
		DEBUG_CHECK((value & mask) == 0, "Flags already set ({0})", mask)
	}
	while (!bits.compare_exchange_weak(value, value | mask));
}


// inline bool trySetFlagsOnce(std::atomic_uint32_t& bits, uint32_t mask) {
// 	uint32_t bits__ = bits.load();

// 	do {
// 		if ((bits__ & mask) == mask) {
// 			return false;
// 		}
// 	}
// 	while (!bits.compare_exchange_weak(bits__, bits__ | mask));

// 	return true;
// }

inline void unsetFlags(std::atomic_uint32_t& bits, uint32_t mask)
{
	uint32_t value = bits.load();
	while (!bits.compare_exchange_weak(value, value & (~mask)));
}

inline size_t coreTaskStorageSize(size_t dataSize)
{
	return CoreTaskSize + ::cold::alignedSize(dataSize, DefaultAlign);
}


struct LockFlagGuard
{
	std::atomic_uint32_t& bits;
	const uint32_t flag;

	LockFlagGuard(std::atomic_uint32_t& bits_, uint32_t flag_): bits(bits_), flag(flag_)
	{
		uint32_t value = bits.load();

		do
		{

			while ((value & flag) != 0)
			{
				value = bits.load();
			}

		}
		while (!bits.compare_exchange_weak(value, value | flag));
	}

	~LockFlagGuard()
	{
		unsetFlags(bits, flag);
	}
};


class TaskRejector final : public CoreTask::Rejector
{
public:

	std::exception_ptr exception() const
	{
		return m_exception;
	}

private:

	void invoke(std::exception_ptr exception_) noexcept override
	{
		m_exception = std::move(exception_);
	}

	std::exception_ptr m_exception;
};


} // namespace


//-----------------------------------------------------------------------------
CoreTaskImpl::CoreTaskImpl(ComPtr<Allocator> allocator_, size_t dataSize_, StateDestructorCallback destructor_)
	: m_allocator(std::move(allocator_))
	, m_dataSize(dataSize_)
	, m_destructor(destructor_)
{}

CoreTaskImpl::~CoreTaskImpl()
{
	if (m_destructor) {
		m_destructor(this->data());
	}
}

CoreTask::TaskId CoreTaskImpl::id() const
{
	constexpr size_t AddressSize = sizeof(void*);
	static_assert(AddressSize <= sizeof(TaskId));

	const void* const address = reinterpret_cast<const void*>(this);
	TaskId id = 0;
	memcpy(&id, &address, AddressSize);
	return id;
}

void CoreTaskImpl::addRef()
{
	RUNTIME_CHECK(m_refs.fetch_add(1ui32) > 0)
}

void CoreTaskImpl::release()
{
	if (m_refs.fetch_sub(1ui32) > 1ui32) {
		return ;
	}

	auto allocator = std::move(m_allocator);
	DEBUG_CHECK(allocator)

	void* const storage = reinterpret_cast<void*>(this);
	std::destroy_at(this);
	allocator->free(storage);
}

bool  CoreTaskImpl::ready() const
{
	return hasFlags(m_flags, TaskFlag_Ready);
}

bool CoreTaskImpl::tryResolve__(ResolverCallback callback, void* ptr) {

	{
		LockFlagGuard lockResolve {m_flags, TaskFlag_ResolveLocked};

		if (hasFlags(m_flags, TaskFlag_Ready))
		{
			return false;
		}

		TaskRejector rejector;
		if (callback)
		{
			callback(rejector, ptr);
			m_exception = rejector.exception();
		}

		setFlagsOnce(m_flags, TaskFlag_Ready);
	}

	invokeReadyCallback();
	tryScheduleContinuation();

	return true;
}

std::exception_ptr CoreTaskImpl::exception() const
{
	DEBUG_CHECK(ready())
	return m_exception;
}

void CoreTaskImpl::rethrow() const
{
	DEBUG_CHECK(ready())
	if (m_exception)
	{
		std::rethrow_exception(m_exception);
	}
}

const void* CoreTaskImpl::data() const
{
	return reinterpret_cast<const std::byte*>(this) + CoreTaskSize;
}

void* CoreTaskImpl::data()
{
	return reinterpret_cast<std::byte*>(this) + CoreTaskSize;
}

size_t CoreTaskImpl::dataSize() const
{
	return m_dataSize;
}

void CoreTaskImpl::setContinuation(TaskContinuation continuation_)
{
	DEBUG_CHECK(!m_continuation)
	m_continuation = std::move(continuation_);
	DEBUG_CHECK(m_continuation)

	setFlagsOnce(m_flags, TaskFlag_HasContinuation);
	tryScheduleContinuation();
}

bool CoreTaskImpl::hasContinuation() const
{
	return hasFlags(m_flags, TaskFlag_HasContinuation);
}

void CoreTaskImpl::setReadyCallback(ReadyCallback callback, void* data1, void* data2)
{
	LockFlagGuard lockReady{m_flags, TaskFlag_ReadyCallbackLocked};

	m_readyCallback = {callback, data1, data2};
	
	if (ready())
	{
		Scheduler::CallbackEntry callback;
		std::swap(callback, m_readyCallback);
		DEBUG_CHECK(!m_readyCallback)

		if (callback)
		{
			callback();
		}
	}
}

void CoreTaskImpl::invokeReadyCallback()
{
	Scheduler::CallbackEntry callback = EXPR_Block
	{
		const LockFlagGuard lockReady{m_flags, TaskFlag_ReadyCallbackLocked};
		Scheduler::CallbackEntry callback = m_readyCallback;
		m_readyCallback = {};
		return callback;
	};

	if (callback)
	{
		callback();
	}
}

void CoreTaskImpl::tryScheduleContinuation()
{
	if (!hasFlags(m_flags, TaskFlag_Ready | TaskFlag_HasContinuation))
	{
		return;
	}

	{
		uint32_t flags = m_flags.load();

		do
		{
			if ((flags & TaskFlag_ContinuationScheduled) == TaskFlag_ContinuationScheduled)
			{
				return;
			}
		}
		while (!m_flags.compare_exchange_weak(flags, flags | TaskFlag_ContinuationScheduled));
	}

	TaskContinuation continuation;
	std::swap(continuation, m_continuation);
	DEBUG_CHECK(!m_continuation)
	DEBUG_CHECK(continuation)

	Scheduler::Ptr scheduler = continuation.scheduler ? std::move(continuation.scheduler) : Scheduler::getCurrent();
	DEBUG_CHECK(scheduler)
	// In any moment right after continuation is scheduled 'this' Task Core instance can be detsructed.
	// Continuation will resume awaiter that holds reference to this task instance and this reference can be released.
	scheduler->schedule(std::move(continuation.invocation));
	// So 'this' task can not be used from this point any more.
}

CoreTaskImpl* CoreTaskImpl::next() const
{
	return m_next;
}

void CoreTaskImpl::setNext(CoreTaskImpl* chain)
{
	m_next = chain;
}


//-----------------------------------------------------------------------------

namespace {

Allocator::Ptr& taskDefaultAllocator()
{

	static Allocator::Ptr taskAllocator = createPoolAllocator(PoolAllocatorConfig {
		.concurrent = true,
		.granularity = 64
	});

	return taskAllocator;
}

} // namespace


CoreTaskPtr CoreTask::create(ComPtr<Allocator> allocator_, size_t dataSize, StateDestructorCallback destructor)
{
	ComPtr<Allocator> allocator = allocator_ ? std::move(allocator_) : taskDefaultAllocator();
	const size_t size = coreTaskStorageSize(dataSize);
	void* const storage = allocator->realloc(nullptr, size);
	DEBUG_CHECK(storage)

	CoreTaskImpl* const coreTask = new (storage) CoreTaskImpl {std::move(allocator), dataSize, destructor};
	return CoreTaskOwnership{coreTask};
}

} // namespace cold::async::core
