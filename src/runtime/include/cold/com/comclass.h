#pragma once
#include <cold/com/comutils.h>
#include <cold/com/ianything.h>
#include <cold/com/refcounted.h>
#include <cold/com/comptr.h>
#include <cold/memory/allocator.h>

#include <type_traits>


namespace cold::com_internal {


template<RefCounted RC>
class ComClassSharedState;


template<RefCounted RC>
class ComClassSharedState final : public IWeakReference
{
public:
	using AcquireFunc = IRefCounted* (*) (void*);
	using DestructorFunc = void (*)(void*);

	ComClassSharedState(ComPtr<Allocator> allocator_, AcquireFunc, DestructorFunc, size_t storageSize);

	void addInstanceRef();

	void releaseInstanceRef();

	uint32_t instanceRefsCount() const;

	IWeakReference* acquireWeakRef();

	ComPtr<Allocator> allocator();


private:

	void releaseStorageRef();

	void addWeakRef() override;

	void release() override;

	IRefCounted* acquire() override;

	bool isDead() const override;


	ComPtr<Allocator> m_allocator;
	AcquireFunc m_acquire;
	DestructorFunc m_destructor;
	const size_t m_storageSize;
	RC m_stateRC;
	RC m_instanceRC;
};

/*!

*/
template<RefCounted RC>
struct ComClassStorage
{
	using SharedState = ComClassSharedState<RC>;

	static constexpr size_t BlockAlignment = sizeof(uintptr_t);
	static constexpr size_t SharedStateSize = sizeof(std::aligned_storage_t<sizeof(SharedState), BlockAlignment>);

	template<typename T>
	static constexpr size_t InstanceStorageSize = SharedStateSize + sizeof(std::aligned_storage_t<sizeof(T), BlockAlignment>);

	template<typename T>
	using InstanceInplaceStorage = std::aligned_storage_t<InstanceStorageSize<T>>;

	template<typename T>
	static SharedState& sharedState(const T& instance)
	{
		const std::byte* const instancePtr = reinterpret_cast<const std::byte*>(&instance);
		std::byte* const statePtr = const_cast<std::byte*>(instancePtr - SharedStateSize);

		return *reinterpret_cast<SharedState*>(statePtr);
	}

	static void* instancePtr(SharedState& state)
	{
		std::byte* const statePtr = reinterpret_cast<std::byte*>(&state);
		return statePtr + SharedStateSize;
	}

	template<typename T>
	static T& instance(SharedState& state)
	{
		std::byte* const statePtr = const_cast<std::byte*>(&state);
		std::byte* const instancePtr = statePtr + SharedStateSize;

		return *reinterpret_cast<T*>(instancePtr);
	}

	template<typename T, typename ... Args>
	static T* createInstance__(void* inplaceMemBlock, ComPtr<Allocator> allocator, Args&&... args)
	{
		DEBUG_CHECK( (!inplaceMemBlock && allocator) || (inplaceMemBlock && !allocator))

		const auto acquire = [](void* instancePtr) -> IRefCounted*
		{
			T& instance = *reinterpret_cast<T*>(instancePtr);
			IRefCounted* refCounted = com::cast<IRefCounted>(instance);
			DEBUG_CHECK(refCounted, "Runtime cast (%1) -> IRefCounted failed", typeid(T))

			return refCounted;
		};

		const auto destructor = [](void* instancePtr)
		{
			T* instance = reinterpret_cast<T*>(instancePtr);
			std::destroy_at(instance);
		};

		constexpr size_t StorageSize = InstanceStorageSize<T>;

		// Allocator or preallocated memory 
		std::byte* const storage = reinterpret_cast<std::byte*>(allocator ? allocator->alloc(StorageSize) : inplaceMemBlock);
		if (!storage)
		{
			return nullptr;
		}

		std::byte* const statePtr = storage;
		std::byte* const instancePtr = statePtr + SharedStateSize;

		[[maybe_unused]] auto sharedState = new (statePtr) SharedState(std::move(allocator), acquire, destructor, StorageSize);

		return new (instancePtr) T(std::forward<Args>(args) ...);
	}

	template<typename T, typename MemBlock, typename ... Args>
	static T* createInstanceInplace(MemBlock& memBlock, Args&&... args)
	{
		static_assert(sizeof(MemBlock) >= InstanceStorageSize<T>);

		void* const ptr = reinterpret_cast<void*>(&memBlock);
		return createInstance__<T>(ptr, ComPtr<Allocator>{}, std::forward<Args>(args)...);
	}

	template<typename T, typename ... Args>
	static T* createInstanceWithAllocator(ComPtr<Allocator> allocator, Args&&... args)
	{
		return createInstance__<T>(nullptr, std::move(allocator), std::forward<Args>(args)...);
	}

	template<typename T, typename ... Args>
	static T* createInstance(Args&&... args)
	{
		return createInstance__<T>(nullptr, defaultAllocator(), std::forward<Args>(args)...);
	}

};

//-----------------------------------------------------------------------------
template<RefCounted RC>
ComClassSharedState<RC>::ComClassSharedState(ComPtr<Allocator> allocator_, AcquireFunc acquire_, DestructorFunc destructor_, size_t storageSize_)
	: m_allocator(std::move(allocator_))
	, m_acquire(acquire_)
	, m_destructor(destructor_)
	, m_storageSize(storageSize_)
{
	DEBUG_CHECK(m_acquire)
	DEBUG_CHECK(m_destructor)
}


 template<RefCounted RC>
ComPtr<Allocator> ComClassSharedState<RC>::allocator()
{
	return this->m_allocator;
}


template<RefCounted RC>
void ComClassSharedState<RC>::addInstanceRef()
{
	m_instanceRC.addRef();
	m_stateRC.addRef();
}


template<RefCounted RC>
void ComClassSharedState<RC>::releaseInstanceRef()
{
	if (m_instanceRC.removeRef() == 1)
	{
		void* const instancePtr = ComClassStorage<RC>::instancePtr(*this);
		m_destructor(instancePtr);
	}

	releaseStorageRef();
}

template<RefCounted RC>
uint32_t ComClassSharedState<RC>::instanceRefsCount() const
{
	return refsCount(m_instanceRC);
}


template<RefCounted RC>
IWeakReference* ComClassSharedState<RC>::acquireWeakRef()
{
	m_stateRC.addRef();
	return this;
}


template<RefCounted RC>
void ComClassSharedState<RC>::releaseStorageRef()
{

	if (m_stateRC.removeRef() == 1)
	{
		DEBUG_CHECK(m_stateRC.noRefs())
		DEBUG_CHECK(m_instanceRC.noRefs())

		auto allocator = std::move(m_allocator);
		const size_t size = m_storageSize;

		std::destroy_at(this);
		if (allocator)
		{
			void* const ptr = reinterpret_cast<void*>(this);
			allocator->free(ptr, size);
		}
	}
}


template<RefCounted RC>
void ComClassSharedState<RC>::addWeakRef()
{
	m_stateRC.addRef();
}


template<RefCounted RC>
void ComClassSharedState<RC>::release()
{
	releaseStorageRef();
}

template<RefCounted RC>
cold::IRefCounted* ComClassSharedState<RC>::acquire()
{
	if (tryAddRef(m_instanceRC))
	{
		m_stateRC.addRef();
		void* const instancePtr = ComClassStorage<RC>::instancePtr(*this);
		IRefCounted* const instance = this->m_acquire(instancePtr);
		return instance;
	}

	return nullptr;
}


template<RefCounted RC>
bool ComClassSharedState<RC>::isDead() const
{
	DEBUG_CHECK(!m_stateRC.noRefs())
	return m_instanceRC.noRefs();
}


} // namespace cold::com_internal


namespace cold::com {

template<RefCounted RCPolicy>
struct ComClassPolicy
{
	using RC = RCPolicy;
};


struct RCPolicy
{
	using Concurrent = cold::ConcurrentRC;
	using SingleThread = cold::SingleThreadRC;
	using StringSingleThread = cold::StrictSingleThreadRC;
};


template<typename T>
inline constexpr size_t InstanceStorageSize = cold::com_internal::ComClassStorage<typename T::ComPolicy::RC>::template InstanceStorageSize<T>;


template<typename T>
using InstanceInplaceStorage = typename cold::com_internal::ComClassStorage<typename T::ComPolicy::RC>::template InstanceInplaceStorage<T>;


template<typename RC = RCPolicy::Concurrent, typename T>
ComPtr<Allocator> getInstanceAllocator(const T& instance)
{
	auto& state = cold::com_internal::ComClassStorage<RC>::template sharedState<T>(instance);
	return state.allocator();
}


template<typename Class_, typename Interface_ = Class_, typename MemBlock, typename ... Args>
ComPtr<Interface_> createInstanceInplace(MemBlock& memBlock, Args&& ... args)
{
	using namespace cold::com_internal;

	Class_* const instance = ComClassStorage<typename Class_::ComPolicy::RC>::template createInstanceInplace<Class_>(memBlock, std::forward<Args>(args)...);
	Interface_* const itf = com::cast<Interface_>(*instance);
	DEBUG_CHECK(itf)
	return com::TakeOwnership(itf);
}


template<typename Class_, typename Interface_ = Class_, typename ... Args>
ComPtr<Interface_> createInstanceSingleton(Args&& ... args)
{
	static com::InstanceInplaceStorage<Class_> storage;

	return com::createInstanceInplace<Class_, Interface_>(storage, std::forward<Args>(args)...);
}


template<typename Class_, typename Interface_ = Class_, typename ... Args>
//requires ComInterface<Interface_>
ComPtr<Interface_> createInstanceWithAllocator(ComPtr<Allocator> allocator, Args&& ... args)
{
	using namespace cold::com_internal;

	Class_* const instance = ComClassStorage<typename Class_::ComPolicy::RC>::template createInstanceWithAllocator<Class_>(std::move(allocator), std::forward<Args>(args)...);
	Interface_* const itf = com::cast<Interface_>(*instance);
	DEBUG_CHECK(itf)
	return com::TakeOwnership(itf);
}


template<typename Class_, typename Interface_ = Class_, typename ... Args>
//requires ComInterface<Interface_>
ComPtr<Interface_> createInstance(Args&& ... args) {
	using namespace cold::com_internal;

	Class_* const instance = ComClassStorage<typename Class_::ComPolicy::RC>::template createInstance<Class_>(std::forward<Args>(args)...);
	Interface_* const itf = com::cast<Interface_>(*instance);
	DEBUG_CHECK(itf)
	return com::TakeOwnership(itf);
}

} // namespace cold::com


//-----------------------------------------------------------------------------
#define IMPLEMENT_ANYTHING\
	\
public:	\
	using cold::IAnything::is;\
	using cold::IAnything::as;\
\
	bool is(const std::type_info& type) const noexcept override {\
		return cold::com::rtIs<decltype(*this)>(type);\
	}\
\
	void* as(const std::type_info& type) const noexcept override {\
		return cold::com::rtCast(*this, type);\
	}\


//#define IMPLEMENT_ANYTHING(base_...)\
//\
//	CLASS_ATTRIBUTES\
//		CLASS_BASE(base_)\
//	END_CLASS_ATTRIBUTES\
//\
//	IMPLEMENT_ANYTHING_METHODS


#define IMPLEMENT_REFCOUNTED(rc_) \
public:\
	using ComPolicy = cold::com::ComClassPolicy<rc_>;\
	using ComClassStorage__ = cold::com_internal::ComClassStorage<ComPolicy::RC>; \
	\
	void addRef() override {\
		ComClassStorage__::sharedState(*this).addInstanceRef();\
	}\
	\
	void release() override {\
		ComClassStorage__::sharedState(*this).releaseInstanceRef();\
	}\
	\
	cold::IWeakReference* getWeakRef() override {\
		return ComClassStorage__::sharedState(*this).acquireWeakRef();\
	}\
	\
	uint32_t refsCount() const override {\
		return ComClassStorage__::sharedState(*this).instanceRefsCount();\
	}\


#define IMPLEMENT_COMCLASS(rc_)\
	IMPLEMENT_REFCOUNTED(rc_) \
	IMPLEMENT_ANYTHING \


#define COMCLASS(rc_, ...)\
	CLASS_ATTRIBUTES\
		CLASS_BASE(__VA_ARGS__)\
	END_CLASS_ATTRIBUTES\
	\
	IMPLEMENT_COMCLASS(rc_) \


#define COMCLASS_(...) COMCLASS(cold::com::RCPolicy::Concurrent, __VA_ARGS__)
