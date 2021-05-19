#pragma once
#include <cold/com/interface.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/preprocessor.h>

#include <typeinfo>
#include <type_traits>

namespace cold {

/**
*/
struct INTERFACE_API IAnything
{
	virtual ~IAnything() = default;
	virtual bool is(const std::type_info&) const noexcept = 0;
	virtual void* as(const std::type_info&) const noexcept = 0;

	template<typename T>
	T as()
	{
		constexpr bool IsPointer = std::is_pointer_v<T>;
		constexpr bool IsReference = std::is_reference_v<T>;

		static_assert(IsPointer || IsReference, "Type for 'as' must be pointer or reference: use 'as<T*>' or 'as<T&>'");
		static_assert(!(IsPointer && IsReference), "Invalid type");

		if constexpr (IsPointer)
		{
			void* const ptr = as(typeid(std::remove_const_t<std::remove_pointer_t<T>>));
			return reinterpret_cast<T>(ptr);
		}
		else
		{
			using Type = std::remove_reference_t<T>;

			void* const ptr = as(typeid(std::remove_const_t<Type>));
			RUNTIME_CHECK(ptr != nullptr, "Typecast failure: [{0}] -> [{1}]", typeid(this), typeid(T));
			return *reinterpret_cast<Type*>(ptr);
		}
	}

	template<typename T/*, std::enable_if_t<std::is_pointer_v<T>, int> = 0*/>
	T as() const requires std::is_pointer_v<T>
	{
		static_assert(std::is_same_v<T, const std::remove_pointer_t<T>* >, "Try to cast through constant instance. T must be const: use 'as<const T*>'");

		return const_cast<IAnything*>(this)->as<std::remove_const_t<T>>();
	}

	template<typename T/*, std::enable_if_t<std::is_reference_v<T>, int> = 0*/>
	T as() const requires std::is_reference_v<T>
	{
		static_assert(std::is_same_v<T, const std::decay_t<T>& >, "Try to cast through constant instance. T must be const: use 'as<const T&>'");

		return const_cast<IAnything*>(this)->as<std::remove_const_t<T>>();
	}

	// template<typename T, std::enable_if_t<!std::is_reference_v<T> && !std::is_pointer_v<T>, int> = 0>
	// T* as() const
	// {
	// 	static_assert(false, "Type for 'as' must be pointer or reference: use 'as<T*>' or 'as<T&>'");
	// 	return reinterpret_cast<T*>(nullptr);
	// }

	template<typename T>
	bool is() const
	{
		static_assert(!(std::is_reference_v<T> || std::is_pointer_v<T> || std::is_const_v<T>), "Invalid requested type");
		return is(typeid(T));
	}
};

/*!
	Ref counted description
*/
struct INTERFACE_API IRefCounted : virtual IAnything
{
	/**
	*/
	virtual void addRef() = 0;

	/**
	*/
	virtual void release() = 0;

	/*!
		Return weak reference
	*/
	virtual struct IWeakReference* getWeakRef() = 0;

	/**
	*/
	virtual uint32_t refsCount() const = 0;
};


/// <summary>
///
/// </summary>
struct INTERFACE_API IWeakReference
{
	virtual void addWeakRef() = 0;

	virtual void release() = 0;

	virtual IRefCounted* acquire() = 0;

	virtual bool isDead() const = 0;
};


namespace com {

class RefCountedGuard
{
public:

	RefCountedGuard(IRefCounted& refCounted_): m_refCounted(&refCounted_)
	{
		DEBUG_CHECK(m_refCounted)
		m_refCounted->addRef();
	}

	RefCountedGuard(const RefCountedGuard&) = delete;

	RefCountedGuard(RefCountedGuard&& other) noexcept: m_refCounted(other.m_refCounted)
	{
		other.m_refCounted = nullptr;
	}

	~RefCountedGuard()
	{
		if (m_refCounted)
		{
			m_refCounted->release();
		}
	}

	RefCountedGuard& operator = (const RefCountedGuard&) = delete;

	RefCountedGuard& operator = (RefCountedGuard&& other) noexcept
	{
		DEBUG_CHECK(!m_refCounted)
		std::swap(m_refCounted, other.m_refCounted);

		return *this;
	}

private:
	IRefCounted* m_refCounted;
};

// template<typename T>
// concept ComInterface = requires(T & itf) {
// 	//dynamic_cast<IRefCounted*>(&itf);
// };

} // namespace com
} // namespace cold


#define KeepRC(rc_)\
	static_assert(!std::is_pointer_v<decltype(rc_)>, "Only reference allowed");\
	static_assert(std::is_base_of_v<cold::IRefCounted, std::remove_reference_t<decltype(rc_)>>, "must be a cold::IRefCounted");\
	\
	const cold::com::RefCountedGuard ANONYMOUS_VARIABLE_NAME(rcGuard__) {static_cast<cold::IRefCounted&>(rc_)}\

