#pragma once

#include "typelist.h"
#include "meta/typetag.h"
#include "utils/typesutility.h"

namespace cold::meta {



namespace typelist_internal {

/// <summary>
///
/// </summary>
template<typename ...T>
struct FirstHelper;


template<typename Head, typename ... Tail>
struct FirstHelper<Head, Tail...>
{
	using type = Head;
};

template<typename ...T>
using First = typename FirstHelper<T...>::type;


/// <summary>
///
/// </summary>
template<typename T>
struct IterateHelper;

/// <summary>
/// Even though F accept generic argument TypeTag<T> can not use "TypeTag<void>{}" wihtin SFINAE expression, because f(TypeTag<T>{}) can be compilable only for T that specified within list.
/// That is why the First<T...> is used - because we need any type, but only from specified Types List.
/// </summary>
template<typename ...T>
struct IterateHelper<TypeList<T...>>
{
	template<typename F>
	constexpr static
	decltype(std::declval<F>()(TypeTag<First<T...>>{}), void()) iterate(F& f)
	{
		(f(TypeTag<T>{}), ...);
	}


	template<typename F>
	constexpr static
	decltype(std::declval<F>()(TypeTag<First<T...>>{}, size_t{}), void()) iterate(F& f)
	{
		size_t index = 0;

		(f(TypeTag<T>{}, index++), ...);
	}


	template<typename F>
	constexpr static
	decltype(std::declval<F>()(TypeTag<First<T...>>{}), bool{}) iterateWithCondition(F& f)
	{
		return (f(TypeTag<T>{}) || ...);
	}


	template<typename F>
	constexpr static
	decltype(std::declval<F>()(TypeTag<First<T...>>{}, size_t{}), bool{}) iterateWithCondition(F& f)
	{
		size_t index = 0;

		return (f(TypeTag<T>{}, index++) || ...);
	}


	template<typename Value, typename F>
	constexpr static
	decltype(std::declval<F>()(TypeTag<First<T...>>{}), std::optional<Value>{}) iterateWithResult(F& f)
	{
		std::optional<Value> result;

		constexpr auto invoker = [](F& f, std::optional<Value>& result, auto tag) -> bool
		{
			if (result)
			{
				return false;
			}

			result = f(tag);

			return !static_cast<bool>(result);
		};

		(invoker(f, result, TypeTag<T>{}) || ...);

		return result;
	}


	template<typename Value, typename F>
	constexpr static
	decltype(std::declval<F>()(TypeTag<First<T...>>{}, size_t{}), std::optional<Value>{}) iterateWithResult(F& f)
	{
		std::optional<Value> result;

		constexpr auto invoker = [](F& f, std::optional<Value>& result, auto tag, size_t index) -> bool
		{
			if (result)
			{
				return false;
			}

			result = f(tag, index);

			return !static_cast<bool>(result);
		};

		size_t index = 0;

		(invoker(f, result, TypeTag<T>{}, index++) || ...);

		return result;
	}
};


template<>
struct IterateHelper<TypeList<>>
{
	template<typename F>
	constexpr static void iterate(F&)
	{}

	template<typename F>
	constexpr static bool iterateWithCondition(F&)
	{
		return false;
	}

	template<typename Value, typename F>
	constexpr static
	std::optional<Value> iterateWithResult(F&)
	{
		return std::nullopt;
	}
};


}

}

