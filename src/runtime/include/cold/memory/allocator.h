#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/com/comptr.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/memory/membase.h>
#include <cold/meta/concepts.h>

#include <optional>
#include <memory_resource>
#include <type_traits>


namespace cold {


/**
*/
struct ABSTRACT_TYPE Allocator : virtual IRefCounted
{
	using Ptr = ComPtr<Allocator>;

	/**
	*/
	virtual void* realloc(void* prevPtr, size_t size, std::optional<size_t> alignment = std::nullopt) = 0;

	/**
	*/
	virtual void free(void* ptr, std::optional<size_t> = std::nullopt) = 0;


	void* alloc(size_t size, std::optional<size_t> alignment = std::nullopt)
	{
		return this->realloc(nullptr, size, alignment);
	}
};


template<typename T>
concept AllocatorProvider = requires
{
	{T::allocator()} -> concepts::AssignableTo<Allocator&>;
};


template<typename T, AllocatorProvider AllocProvider>
class StdAllocator
{
public:
	using Provider = AllocProvider;

	using value_type = T;
	using propagate_on_container_move_assignment = std::true_type;

	StdAllocator() noexcept = default;

	template<typename U, AllocatorProvider UProvider>
	StdAllocator(const StdAllocator<U, UProvider>&) noexcept
	{}

	[[nodiscard]] constexpr T* allocate(size_t n)
	{
		void* const ptr = Provider::allocator().realloc(nullptr, sizeof(T) * n, alignof(T));
		return reinterpret_cast<T*>(ptr);
	}

	void deallocate(T* p, std::size_t n)
	{
		const size_t size = sizeof(T) * n;
		Provider::allocator().free(reinterpret_cast<void*>(p), size);
	}

	template<typename U, AllocatorProvider UProvider>
	bool operator == (const StdAllocator<U, UProvider>&) const
	{
		return &UProvider::allocator() = &Provider<T>::allocator();
	}

	template<typename U, AllocatorProvider UProvider>
	bool operator!= (const StdAllocator<U, UProvider>&) const
	{
		return &UProvider::allocator() = &Provider<T>::allocator();
	}
};


//struct RUNTIME_EXPORT ThreadDefaultAllocatorScope
//{
//	ThreadDefaultAllocatorScope(Allocator::Ptr);
//
//	~ThreadDefaultAllocatorScope();
//};


template<typename Class_>
struct SingletonMemOp
{
	static void* operator_new(size_t) noexcept
	{
		decltype(auto) state = getSingletonState();
		DEBUG_CHECK(!state.allocated, "Signleton ({0}) already allocated", typeid(Class_))

		state.allocated = true;
		return &state.storage;
	}

	static void operator_delete(void* ptr, size_t) noexcept
	{
		decltype(auto) state = getSingletonState();
		DEBUG_CHECK(state.allocated)
		state.allocated = false;
	}

private:

	static decltype(auto) getSingletonState() noexcept
	{
		static struct
		{
			std::aligned_storage_t<sizeof(Class_)> storage;
			bool allocated = false;
		}
		state {};

		return (state);
	};
};

#define SINGLETON_MEMOP(ClassName)\
public:\
	static void* operator new (size_t size) \
	{\
		return ::cold::SingletonMemOp<ClassName>::operator_new(size);\
	}\
	\
	static void operator delete (void* ptr, size_t size) \
	{\
		::cold::SingletonMemOp<ClassName>::operator_delete(ptr, size);\
	}\


#if 0

template<typename Provider>
requires (std::is_invoca
class StdAllocatorWrapper
{
public:
	using value_type = T;

	using propagate_on_container_move_assignment = std::true_type;
	using is_always_equal = std::true_type;

	StdAllocatorWrapper(ComPtr<Allocator> allocator_) noexcept: allocator(std::move(allocator_))
	{}

	StdAllocatorWrapper(const StdAllocatorWrapper<T>& other): allocator(other.allocator)
	{}

	StdAllocatorWrapper(StdAllocatorWrapper<T>&& other): allocator(std::move(other.allocator))
	{}

	template<typename U>
	StdAllocatorWrapper(const StdAllocatorWrapper<U>& other): allocator(other.allocator)
	{}

	template<typename U>
	StdAllocatorWrapper(StdAllocatorWrapper<U>&& other): allocator(std::move(other.allocator))
	{}

	[[nodiscard]] constexpr T* allocate(size_t n) {
		DEBUG_CHECK(this->allocator)
		void* const ptr = allocator->realloc(nullptr, sizeof(T) * n);
		return reinterpret_cast<T*>(ptr);
	}

	void deallocate(T* p, std::size_t n) {
		DEBUG_CHECK(this->allocator)
		const size_t blockSize = sizeof(T) * n;
		this->allocator->free(reinterpret_cast<void*>(p), blockSize);
	}

	bool operator == (const StdAllocatorWrapper<T>& other) const {
		return other.allocator.get() == this->allocator.get();
	}

	bool operator!= (const StdAllocatorWrapper<T>& other) const {
		return other.allocator.get() != this->allocator.get();
	}
};
#endif


class RUNTIME_EXPORT AllocatorMemoryResource : public std::pmr::memory_resource
{
public:
	using AllocatorProvider = Allocator* (*)();

	AllocatorMemoryResource(AllocatorProvider allocProvider);

private:

	void* do_allocate(size_t size_, size_t align_) override;
	void do_deallocate(void* ptr_, size_t size_, size_t align_) override;
	bool do_is_equal(const std::pmr::memory_resource& other_) const noexcept override;
	Allocator& allocator() const;


	AllocatorProvider m_allocatorProvider;
};



struct PoolAllocatorConfig
{
	static constexpr size_t DefaultGranularity = 64;
	static constexpr size_t DefaultBlockMinSize = 32;
	static constexpr size_t DefaultBlockMaxSize = 1024;

	const bool concurrent = true;
	const size_t granularity = DefaultGranularity;
	const size_t blockMinSize = DefaultBlockMinSize;
	const size_t blockMaxSize = DefaultBlockMaxSize;
};



RUNTIME_EXPORT Allocator::Ptr crtAllocator();

RUNTIME_EXPORT Allocator::Ptr defaultAllocator();

RUNTIME_EXPORT void setDefaultAllocator(ComPtr<Allocator>);

RUNTIME_EXPORT Allocator::Ptr defaultAllocator();

RUNTIME_EXPORT void setDefaultAllocator(ComPtr<Allocator>);

RUNTIME_EXPORT Allocator::Ptr createPoolAllocator(PoolAllocatorConfig = {});

}
