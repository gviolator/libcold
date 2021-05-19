#pragma once
#include <cold/utils/typeutility.h>

#include <type_traits>


namespace cold::concepts {


template<typename Expr, typename U>
concept Same = std::is_same_v<Expr, U>;


template<typename Expr, typename Base_>
concept Base = std::is_base_of_v<Base_, Expr>;


template<typename Expr, typename Arg>
concept ConstructibleFrom = std::is_constructible_v<Expr, Arg>;


template<typename Expr, typename T>
concept AssignableTo = std::is_assignable_v<T, Expr>;


template<template <typename, auto ...> class F>
concept MetaFunction = requires
{
	typename F<int>::type;
};


template<typename Expr, template<typename ... > class TemplateClass>
concept TemplateOf = IsTemplateOf<TemplateClass, Expr>;


template<typename T,  typename Target>
concept StaticCast = requires (T& instance) {
	static_cast<Target&>(instance);
};


}
