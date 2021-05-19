#pragma once
#include <cold/com/comutils.h>
#include <cold/diagnostics/logging.h>
#include <cold/utils/disposable.h>
#include <cold/utils/nothing.h>


namespace cold {
namespace com_internal {


template<typename T>
inline IRefCounted& asRefCounted(T& instance)
{
	[[maybe_unused]] IRefCounted* rc = nullptr;

	if constexpr (std::is_convertible_v<T*, IRefCounted*>)
	{
		return instance;
	}
	else if constexpr (std::is_convertible_v<T*, IAnything*>)
	{
		rc = static_cast<IAnything&>(instance).as<IRefCounted*>();
	}
	else if (rc = cold::com::cast<IRefCounted>(instance); !rc)
	{
		rc = dynamic_cast<IRefCounted*>(&instance);
	}

	DEBUG_CHECK(rc, "Runtime can not find IRefCounted for (%1)", typeid(T))

	return *rc;
}

} // com_internal


namespace com {

/// <summary>
///
/// </summary>
template<typename T>
struct Acquire
{
	T* const ptr;

	Acquire(T* ptr_) : ptr(ptr_)
	{}

	Acquire(const Acquire&) = delete;
};


/// <summary>
///
/// </summary>
template<typename T>
struct TakeOwnership
{
	T* const ptr;

	TakeOwnership(T* ptr_) : ptr(ptr_)
	{}

	TakeOwnership(const TakeOwnership&) = delete;
};

} // namespace com

template<typename T = IRefCounted>
class ComPtr
{
public:

	using type = T;

	ComPtr() = default;

	ComPtr(Nothing)
	{}

	ComPtr(const ComPtr<T>& other): m_instance(other.m_instance)
	{
		if (m_instance != nullptr)
		{
			com_internal::asRefCounted(*m_instance).addRef();
		}
	}

	ComPtr(ComPtr<T>&& other): m_instance(other.giveup())
	{}

	ComPtr(const com::Acquire<T>& acquire_): m_instance(acquire_.ptr)
	{
		if (m_instance)
		{
			com_internal::asRefCounted(*m_instance).addRef();
		}
	}

	ComPtr(const com::TakeOwnership<T>& ownership_): m_instance(ownership_.ptr)
	{}

	template<typename U>
	requires (!std::is_same_v<U,T>)
	ComPtr(const ComPtr<U>& other)
	{
		//static_assert(std::is_convertible_v<U&, T&> || std::is_same_v<T, com::IRefCountedObject> || std::is_same_v<U, com::IRefCountedObject>, "Unsafe type cast");
		this->acquire(other.get());
	}

	template<typename U>
	requires (!std::is_same_v<U,T>)
	ComPtr(ComPtr<U>&& other)
	{
		//static_assert(std::is_convertible_v<U&, T&> || std::is_same_v<T, com::IRefCountedObject> || std::is_same_v<U, com::IRefCountedObject>, "Unsafe type cast");
		this->moveAcquire(other.giveup());
	}

	~ComPtr() {
		this->release();
	}

	T* giveup()
	{
		T* const instance = m_instance;
		m_instance = nullptr;

		return instance;
	}

	T* get() const
	{
		return m_instance;
	}

	void reset(T* ptr = nullptr)
	{
		acquire(ptr);
	}

	ComPtr<T>& operator = (const ComPtr<T>& other)
	{
		this->acquire(other.m_instance);
		return *this;
	}

	ComPtr<T>& operator = (ComPtr<T>&& other)
	{
		this->moveAcquire(other.giveup());
		return *this;
	}

	template<typename U>
	requires (!std::is_same_v<U,T>)
	ComPtr<T>& operator = (const ComPtr<U>& other)
	{
		U* instance = other.get();
		this->acquire<U>(instance);
		return *this;
	}

	template<typename U>
	requires (!std::is_same_v<U,T>)
	ComPtr<T>& operator = (ComPtr<U>&& other)
	{
		this->moveAcquire<U>(other.giveup());
		return *this;
	}

	T& operator * () const
	{
		RUNTIME_CHECK(m_instance, "ComPtr<%1> is not dereferencible", typeid(T))
		return *m_instance;
	}

	T* operator -> () const
	{
		RUNTIME_CHECK(m_instance , "ComPtr<%1> is not dereferencible", typeid(T))

		return m_instance;
	}

	explicit operator bool() const
	{
		return m_instance != nullptr;
	}


private:

	void release()
	{
		if (m_instance)
		{
			T* const temp = m_instance;
			m_instance = nullptr;
			com_internal::asRefCounted(*temp).release();
		}
	}

	void acquire(T* instance)
	{
		T* const prevInstance = m_instance;

		if (m_instance = instance; m_instance)
		{
			com_internal::asRefCounted(*m_instance).addRef();
		}

		if (prevInstance)
		{
			com_internal::asRefCounted(*prevInstance).release();
		}
	}

	void moveAcquire(T* instance)
	{
		release();
		m_instance = instance;
	}

	template<typename U>
	requires (!std::is_same_v<U,T>)
	void acquire(U* instance)
	{
		release();
		if (instance == nullptr)
		{
			return;
		}
		
		if (m_instance = com_internal::asRefCounted(*instance).template as<T*>(); m_instance != nullptr)
		{
			com_internal::asRefCounted(*m_instance).addRef();
		}
		else
		{
			LOG_warning_("Expected API not exposed: (%2). Source type (%1)", typeid(U), typeid(T))
#if (defined(DEBUG) || !defined(NDEBUG)) && defined(_APISETDEBUG_)
			if (IsDebuggerPresent() == TRUE)
			{
				DebugBreak();
			}
#endif
		}
	}

	template<typename U>
	requires (!std::is_same_v<U,T>)
	void moveAcquire(U* instance)
	{
		release();
		if (instance == nullptr)
		{
			return;
		}

		if (m_instance = com_internal::asRefCounted(*instance).template as<T*>(); m_instance == nullptr)
		{
			LOG_warning_("Expected API not exposed: (%2). Source type (%1)", typeid(U), typeid(T))

#if (defined(DEBUG) || !defined(NDEBUG)) && defined(_APISETDEBUG_)
			if (IsDebuggerPresent() == TRUE)
			{
				DebugBreak();
			}
#endif
			com_internal::asRefCounted(*instance).release();
		}
	}

	T* m_instance = nullptr;
};


template<typename T>
ComPtr(com::Acquire<T>) -> ComPtr<T>;

template<typename T>
ComPtr(com::TakeOwnership<T>) -> ComPtr<T>;


template<typename T>
inline void dispose(ComPtr<T>& ptr) requires (std::is_assignable_v<Disposable&, T&>)
{
	if (ptr)
	{
		Disposable& disposable = static_cast<Disposable&>(*ptr);
		disposable.dispose();
	}
}

template<typename T>
inline void dispose(ComPtr<T>& ptr)
{
	if (Disposable* const disposable = ptr ? ptr->template as<Disposable*>() : nullptr; disposable)
	{
		disposable->dispose();
	}
}

template<typename T>
class DisposableGuard<ComPtr<T>> : protected ComPtr<T>
{
public:
	using ComPtr<T>::ComPtr;
	using ComPtr<T>::operator ->;
	using ComPtr<T>::operator *;

	DisposableGuard(ComPtr<T> ptr): ComPtr<T>(std::move(ptr))
	{}

	~DisposableGuard()
	{
		dispose(static_cast<ComPtr<T>&>(*this));
	}
};


template<typename T>
DisposableGuard(ComPtr<T>) -> DisposableGuard<ComPtr<T>>;


} // namespace cold


#define using_(var_) ::cold::DisposableGuard var_
