#pragma once
#include "typelist.h"

namespace cold {
namespace typelist_internal {

/// <summary>
///
/// </summary>
template<typename ...T>
struct Concat2TypeLists;


template<typename TL>
struct Concat2TypeLists<TL> {
	static_assert(typelist::IsTypeList<TL>);
	using type = TL;
};


template<typename ...T1, typename ...T2>
struct Concat2TypeLists<TypeList<T1...>, TypeList<T2...>> {
	using type = TypeList<T1..., T2...>;
};

/// <summary>
///
/// </summary>
template<typename ...>
struct Concat__;


template<>
struct Concat__<> {
	using type = TypeList<> ;
};

template<typename TL, typename ... Tail>
struct Concat__<TL, Tail...>
{
	using type = typename cold::typelist_internal::Concat2TypeLists<TL, typename Concat__<Tail...>::type>::type;
};


}

namespace typelist {


template<typename ... TL>
using Concat = typename cold::typelist_internal::Concat__<TL...>::type;


} // namespace typelist
} // namespace cold
