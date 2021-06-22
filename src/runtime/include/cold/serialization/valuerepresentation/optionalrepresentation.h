#pragma once
#include "representationbase.h"
#include <cold/diagnostics/runtimecheck.h>
#include <cold/meta/concepts.h>
#include <cold/utils/typeutility.h>

namespace cold::meta {

template<typename T>
struct OptionalValueOperations;

template<typename T>
concept OptionalRepresentable = 
	requires(T& opt)
	{
		OptionalValueOperations<T>::reset(opt);
		OptionalValueOperations<T>::value(opt);
	} &&
	requires(const T& opt)
	{
		{OptionalValueOperations<T>::hasValue(opt)} -> concepts::Same<bool>;
		OptionalValueOperations<T>::value(opt);
	};

#if 0
/**

*/
template<OptionalRepresentable T>
class OptionalRepresentation final : RepresentationBase<T>
{
public:
	using OptionalType = std::remove_const_t<T>;
	using WrappedType = typename OptionalValueOperations<OptionalType>::ValueType;

	OptionalRepresentation(OptionalType& opt_): m_optional(opt_)
	{}

	bool hasValue() const
	{
		return OptionalValueOperations<OptionalType>::hasValue(m_optional);
	}

	explicit operator bool () const
	{
		return this->hasValue();
	}

	auto value()
	{
		decltype(auto) valueRef = OptionalValueOperations<OptionalType>::value(m_optional);
		static_assert(std::is_reference_v<decltype(valueRef)>);
		return represent(valueRef);
	}

	void reset()
	{
		OptionalValueOperations<OptionalType>::reset(m_optional);
	}

	template<typename ... Args>
	auto emplace(Args&& ... args)
	{
		decltype(auto) valueRef = OptionalValueOperations<OptionalType>::emplace(m_optional, std::forward<Args>(args)...);
		static_assert(std::is_reference_v<decltype(valueRef)>);

		return represent(valueRef);
	}

private:
	OptionalType& m_optional;
};

/**
*/
template<OptionalRepresentable T>
class ConstOptionalRepresentation final : RepresentationBase<T>
{
public:
	using OptionalType = std::remove_const_t<T>;
	using WrappedType = typename OptionalValueOperations<OptionalType>::ValueType;

	ConstOptionalRepresentation(const OptionalType& opt_): m_optional(opt_)
	{}

	bool hasValue() const
	{
		return OptionalValueOperations<OptionalType>::hasValue(m_optional);
	}

	explicit operator bool () const
	{
		return this->hasValue();
	}

	auto value() const {
		decltype(auto) valueRef = OptionalValueOperations<OptionalType>::value(m_optional);
		static_assert(std::is_reference_v<decltype(valueRef)>);
		return represent(valueRef);
	}

private:
	const OptionalType& m_optional;
};

//-----------------------------------------------------------------------------

template<OptionalRepresentable T>
OptionalRepresentation<T> represent(T& value)
{
	return value;
}

template<OptionalRepresentable T>
ConstOptionalRepresentation<T> represent(const T& value)
{
	return value;
}

#endif

} // namespace cold::meta