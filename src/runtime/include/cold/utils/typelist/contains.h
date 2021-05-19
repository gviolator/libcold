#pragma once
#include "typelist.h"

namespace cold::typelist {
namespace typelist_internal {

template<typename TL, typename El>
struct Contains__;


template<typename ... T, typename El>
struct Contains__<TypeList<T...>, El> {
	constexpr static bool value = (std::is_same_v<T, El> || ...);
};

template<typename TL, typename SubList>
struct ContainsAll__;

template<typename TL, typename ... T>
struct ContainsAll__<TL, TypeList<T...>> {
	constexpr static bool value = (Contains__<TL, T>::value && ...);
};

} // namespace typelist_internal

template<typename TL, typename T>
inline constexpr bool Contains = typelist_internal::Contains__<TL, T>::value;

template<typename TL, typename SubList>
inline constexpr bool ContainsAll = typelist_internal::ContainsAll__<TL, SubList>::value;

}
