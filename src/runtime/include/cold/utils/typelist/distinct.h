#pragma once
#include "typelist.h"

namespace cold::typelist_internal {

/// <summary>
///
/// </summary>
template<typename TL, typename Result = TypeList<>>
struct Distinct__;


template<typename Result>
struct Distinct__<TypeList<>, Result> {
	using type = Result;
};


template<typename Element, typename ... Tail, typename ... Result>
struct Distinct__<TypeList<Element, Tail...>, TypeList<Result...>>
{
	static constexpr bool NotExists = typelist::ElementIndex<TypeList<Result...>, Element> < 0;

	using type = std::conditional_t<NotExists,
		typename Distinct__<TypeList<Tail...>, TypeList<Result..., Element>>::type,
		typename Distinct__<TypeList<Tail...>, TypeList<Result...>>::type
	>;
};

} //namespace cold::typelist_internal

namespace cold::typelist {

template<typename TL>
using Distinct = typename cold::typelist_internal::Distinct__<TL>::type;

}
