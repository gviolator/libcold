#pragma once
#include "representationbase.h"
#include <cold/meta/concepts.h>
#include <cold/utils/typeutility.h>
#include <optional>


namespace cold::meta {


template <typename>
struct ArrayValueOperations;

template<typename T>
concept ArrayRepresentable =
	requires(T& arr)
{
		ArrayValueOperations<T>::element(arr, size_t{0});
		ArrayValueOperations<T>::clear(arr, std::optional<size_t>{});
		ArrayValueOperations<T>::emplaceBack(arr);

	} && requires(const T& arr)
	{
		{ArrayValueOperations<T>::size(arr)} -> concepts::AssignableTo<size_t&>;
		ArrayValueOperations<T>::element(arr, size_t{0});
	};


template<typename T>
using ArrayRepresentationElementType = std::decay_t<decltype(ArrayValueOperations<T>::element(std::declval<T>(), size_t{0}))>;

/// <summary>
///
/// </summary>
template<ArrayRepresentable T>
class ArrayRepresentation final : public RepresentationBase<T,T>
{
public:

	using ElementType = ArrayRepresentationElementType<T>;

	ArrayRepresentation(T& a): m_array(a)
	{}

	size_t size() const
	{
		return ArrayValueOperations<T>::size(m_array);
	}

	auto operator[](size_t index) const
	{
		decltype(auto) element = ArrayValueOperations<T>::element(m_array, index);
		static_assert(std::is_reference_v<decltype(element)>);
		return represent(element);
	}


	void clear(std::optional<size_t> reserveSize = std::nullopt)
	{
		ArrayValueOperations<T>::clear(m_array, reserveSize);
	}

	template<typename ... U>
	auto emplaceBack(U&& ... values)
	{
		decltype(auto) element = ArrayValueOperations<T>::emplaceBack(m_array);
		static_assert(std::is_reference_v<decltype(element)>);
		return represent(element);
	}

private:

	T& m_array;
};


/// <summary>
///
/// </summary>
template<ArrayRepresentable T>
class ConstArrayRepresentation final : public RepresentationBase<T,T>
{
public:

	using ElementType = ArrayRepresentationElementType<T>;

	ConstArrayRepresentation(const T& a): m_array(a)
	{}


	size_t size() const
	{
		return ArrayValueOperations<T>::size(m_array);
	}

	auto operator[](size_t index) const
	{
		decltype(auto) element = ArrayValueOperations<T>::element(m_array, index);
		static_assert(std::is_reference_v<decltype(element)>);
		return represent(element);
	}

private:
	const T& m_array;
};

//-----------------------------------------------------------------------------

template<ArrayRepresentable T>
ArrayRepresentation<T> represent(T& container)
{
	return container;
}

template<ArrayRepresentable T>
ConstArrayRepresentation<T> represent(const T& container)
{
	return container;
}

template<typename T>
inline constexpr bool IsArrayRepresentation = IsTemplateOf<ArrayRepresentation, T>;

template<typename T>
inline constexpr bool IsConstArrayRepresentation = IsTemplateOf<ConstArrayRepresentation, T>;

} // namespace cold::meta
