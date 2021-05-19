#pragma once
#include "comptr.h"

namespace cold {


template<typename T = IRefCounted>
class WeakComPtr
{
public:
	using type = T;


	WeakComPtr() = default;

	WeakComPtr(const WeakComPtr<T> & weakPtr): m_weakRef(weakPtr.m_weakRef)
	{
		if (m_weakRef)
		{
			m_weakRef->addWeakRef();
		}
	}

	WeakComPtr(WeakComPtr<T> && weakPtr): m_weakRef(weakPtr.m_weakRef)
	{
		weakPtr.m_weakRef = nullptr;
	}

	explicit WeakComPtr(const ComPtr<T>& ptr)
	{
		if (ptr)
		{
			m_weakRef = com_internal::asRefCounted(*ptr).getWeakRef();
		}
	}

	~WeakComPtr()
	{
		reset();
	}

	WeakComPtr<T>& operator = (const WeakComPtr<T> & other)
	{
		reset();

		if (m_weakRef = other.m_weakRef; m_weakRef)
		{
			m_weakRef->addWeakRef();
		}

		return *this;
	}

	WeakComPtr<T>& operator = (WeakComPtr<T>&& other)
	{
		reset();

		DEBUG_CHECK(m_weakRef == nullptr)
		std::swap(m_weakRef, other.m_weakRef);

		return *this;
	}

	WeakComPtr<T>& operator = (const ComPtr<T>& ptr)
	{
		reset();

		if (ptr)
		{
			m_weakRef = com_internal::asRefCounted(*ptr).getWeakRef();
		}

		return *this;
	}

	ComPtr<T> acquire()
	{
		IRefCounted* const ptr = (m_weakRef != nullptr) ? m_weakRef->acquire() : nullptr;

		if (!ptr)
		{
			return {};
		}

		T* const targetInstance = ptr->as<T*>();

		DEBUG_CHECK(targetInstance, "RefCounted object acquired through weak reference, but instance doesn't provide target interface (%1)", typeid(T))

		return com::TakeOwnership{ targetInstance };
	}

	explicit operator bool() const
	{
		return m_weakRef != nullptr;
	}

	/// <summary>
	/// Check that referenced instance is not accessible anymore (i.e. it has been destructed).
	/// System can guarantee - that died instance never gone to be alive,
	/// but in opposed case alive instance can be a dead immediately right after isAlive returns true;
	/// <summary>
	bool isDead() const
	{
		return !m_weakRef || m_weakRef->isDead();
	}

	void reset()
	{
		if (m_weakRef)
		{
			IWeakReference* weakRef = nullptr;
			std::swap(weakRef, m_weakRef);
			weakRef->release();
		}
	}

	IWeakReference* get() const
	{
		return m_weakRef;
	}

	IWeakReference* giveup()
	{
		IWeakReference* const weakRef = m_weakRef;
		m_weakRef = nullptr;
		return weakRef;
	}

private:

	IWeakReference* m_weakRef = nullptr;
};


}
