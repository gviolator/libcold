#pragma once

#include "typelist.h"


namespace cold::meta::typelist_internal {


/// <summary>
///
/// </summary>
template<typename TL, template<typename> class Predicate, typename Result = TypeList<>>
struct Filter;


template<template <typename> class Predicate, typename Result>
struct Filter<TypeList<>, Predicate, Result>
{
	using type = Result;
};


template<typename Current, typename ... Tail, template<typename> class Predicate, typename ... Result>
struct Filter<TypeList<Current, Tail...>, Predicate, TypeList<Result...>>
{
	using type = std::conditional_t<Predicate<Current>::value,
		typename Filter<TypeList<Tail...>, Predicate, TypeList<Result..., Current>>::type,
		typename Filter<TypeList<Tail...>, Predicate, TypeList<Result...>>::type
	>;
};


}
