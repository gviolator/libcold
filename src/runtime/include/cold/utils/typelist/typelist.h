#pragma once
#include <type_traits>
#include <tuple>

namespace cold {

/// <summary>
///
/// </summary>
template<typename ...T>
struct TypeList {
	static constexpr size_t Size = sizeof ... (T);
};

namespace typelist_internal {

template<typename>
struct TypeListOf__;

template<typename...T>
struct TypeListOf__<std::tuple<T...>> {
	using type = TypeList<T...>;
};

/// <summary>
///
/// </summary>
template<typename TL, typename T, int Index>
struct ElementIndex__;


template<typename T, int Index>
struct ElementIndex__<TypeList<>, T, Index> {
	inline constexpr static int value = -1;
};


template<typename Head, typename ... Tail, typename T, int Index>
struct ElementIndex__<TypeList<Head, Tail...>, T, Index>
{
	inline constexpr static int value = std::is_same_v<T, Head> ?
		Index :
		ElementIndex__<TypeList<Tail...>, T, Index + 1>::value;
};


template<typename T>
struct IsTypeList__ : std::false_type
{};

template<typename ...U>
struct IsTypeList__<TypeList<U...>> : std::true_type
{};


} // namespace typelist_internal

namespace typelist {

template<typename T>
using TypeListOf = typename cold::typelist_internal::TypeListOf__<T>::type;

template<typename TL, typename T>
inline constexpr int ElementIndex = cold::typelist_internal::ElementIndex__<TL, T, 0>::value;

template<typename T>
inline constexpr bool IsTypeList = cold::typelist_internal::IsTypeList__<T>::value;


} // namespace typelist
} // namespace cold
