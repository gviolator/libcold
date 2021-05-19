#pragma once
#include <cold/serialization/valuerepresentation/representationbase.h>
#include <cold/utils/typeutility.h>

namespace cold::meta {


template<typename T>
const T& getPrimitiveValue(const T& value_) {
	return value_;
}


template<typename T, typename U>
requires std::is_assignable_v<T&, U>
void setPrimitiveValue(T& instance, U&& value_) {
	instance = std::forward<U>(value_);
}

template<typename T, typename U>
requires requires (T target_, const U& value_) {
	setPrimitiveValue(target_, value_);
}
std::true_type setPrimitiveValueValidator__();

template<typename T, typename U>
std::false_type setPrimitiveValueValidator__();


template<typename T>
using PrimitiveRepresentationValueType = std::decay_t<decltype(getPrimitiveValue(std::declval<T>()))>;

template<typename T, typename U>
constexpr bool IsPrimitiveAssignable = decltype(setPrimitiveValueValidator__<T,U>())::value;


template<typename T>
class PrimitiveRepresentation : public RepresentationBase<T, PrimitiveRepresentationValueType<T>>
{
public:

	PrimitiveRepresentation(T& value): m_value(value)
	{}

	template<typename U>
	void setValue(U&& value) {
		static_assert(!std::is_const_v<T>);
		setPrimitiveValue(m_value , std::forward<U>(value));
	}

	decltype(auto) value() const {
		return getPrimitiveValue(m_value);
	}

private:

	T& m_value;
};


/// <summary>
///
/// </summary>
template<typename T>
class ConstPrimitiveRepresentation : public RepresentationBase<T, PrimitiveRepresentationValueType<T>>
{
public:

	ConstPrimitiveRepresentation(const T& value): m_value(value)
	{}

	decltype(auto) value() const {
		return getPrimitiveValue(m_value);
	}

private:

	const T& m_value;
};

//-----------------------------------------------------------------------------

template<typename T>
const ConstPrimitiveRepresentation<T> represent(const T& value) {
	return {value};
}

template<typename T>
PrimitiveRepresentation<T> represent(T& value) {
	return {value};
}

template<typename T>
inline constexpr bool IsPrimitiveRepresentation = IsTemplateOf<PrimitiveRepresentation, T>;

template<typename T>
inline constexpr bool IsConstPrimitiveRepresentation = IsTemplateOf<ConstPrimitiveRepresentation, T>;


} // namespace cold::meta



