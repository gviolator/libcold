#pragma once

#include <cold/utils/typelist.h>
#include <cold/meta/concepts.h>



namespace cold::typelist_internal {

/// <summary>
///
/// </summary>
template<typename TL, template <typename, auto...> class Mapper>
struct Transform__;


template<template <typename, auto...> class Mapper, typename ... T>
struct Transform__<TypeList<T...>, Mapper>
{
	using type = TypeList<Mapper<T> ... >;
};


template<template <typename, auto...> class Mapper, typename ... T>
requires concepts::MetaFunction<Mapper>
struct Transform__<TypeList<T...>, Mapper>
{
	using type = TypeList<typename Mapper<T>::type ... >;
};


}

namespace cold::typelist {

template<typename TL, template <typename, auto...> class Mapper>
using Transform = typename cold::typelist_internal::Transform__<TL, Mapper>::type;


}

