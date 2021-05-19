#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/memory/allocator.h>

#include <memory_resource>
#include <optional>

namespace cold {


RUNTIME_EXPORT void* runtimePoolAllocate(size_t size) noexcept;

RUNTIME_EXPORT void runtimePoolFree(void* ptr, std::optional<size_t> = std::nullopt) noexcept;

RUNTIME_EXPORT Allocator::Ptr& runtimePoolAllocator();

RUNTIME_EXPORT std::pmr::memory_resource* runtimePoolMemoryResource();

struct RuntimeAllocatorProvider
{
	static inline Allocator& allocator()
	{
		return *runtimePoolAllocator().get();
	}
};

template<typename T>
using RuntimeStdAllocator = StdAllocator<T, RuntimeAllocatorProvider>;

}


#if 0
#include <memory>


namespace cold {

/**
*/
struct INTERFACE_API IStackAllocatorProvider
{
	enum class AllocationBudget
	{
		Default,
		Big
	};

	//static RUNTIME_EXPORT std::unique_ptr<IStackAllocatorProvider> create(ComPtr<Allocator> fallbackAllocator = {});
	static RUNTIME_EXPORT bool exists();
	static RUNTIME_EXPORT IStackAllocatorProvider& instance();



	virtual ~IStackAllocatorProvider() = default;

	virtual ComPtr<Allocator> acquireAllocator(AllocationBudget = AllocationBudget::Default) = 0;
};


RUNTIME_EXPORT void* runtimeAllocate(size_t size);

RUNTIME_EXPORT ComPtr<Allocator> runtimeAllocator();



template <class T>
class RuntimeStdAllocator
{
public:

	using value_type = T;
	using size_type = size_t;
	using difference_type = ptrdiff_t;

	using propagate_on_container_move_assignment = std::true_type;
	using is_always_equal = std::true_type;

	constexpr RuntimeStdAllocator() noexcept {}

	constexpr RuntimeStdAllocator(const RuntimeStdAllocator&) noexcept = default;

	template <class U>
	constexpr RuntimeStdAllocator(const RuntimeStdAllocator<U>&) noexcept {}

	void deallocate([[maybe_unused]] T* ptr, [[maybe_unused]] size_t count) {
	}

	[[nodiscard]] __declspec(allocator) T* allocate(const size_t count) {
		void* const ptr = runtimeAllocate(sizeof(T) * count);
		return reinterpret_cast<T*>(ptr);
	}
};

}

#define RUNTIME_ALLOCATE \
	static void* operator new (size_t size) {\
		return ::cold::runtimeAllocate(size);\
	}\
	\
	static void operator delete (void*, size_t) {\
	}\

#endif


#define RUNTIME_POOL_ALLOCATE \
	static void* operator new (size_t size) {\
		return ::cold::runtimePoolAllocate(size);\
	}\
	\
	static void operator delete (void* ptr, size_t size) {\
		::cold::runtimePoolFree(ptr, size); \
	}\
