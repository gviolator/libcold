#pragma once
#include <cold/diagnostics/runtimecheck.h>

#include <type_traits>
#include <tuple>
#include <utility>

namespace cold {


struct Tuple
{

	template<typename T>
	using Indexes = std::make_index_sequence<std::tuple_size_v<std::decay_t<T>>>;

	template<typename T>
	static constexpr auto indexes(const T&)
	{
		return Indexes<T>{};
	}

	/// <summary>
	///
	/// </summary>
	template<typename T, typename Element>
	constexpr static bool contains()
	{
		constexpr auto contains__ = [] <size_t ... I>(std::index_sequence<I...>)
		{
			return (std::is_same_v<Element, El<T, I>> || ...);
		};
		
		return contains__(Indexes<T>{});
	}


	template<typename Tuple_, typename F>
	static void invokeAt(Tuple_& tuple_, size_t index, F accessor)
	{
		constexpr size_t Size = std::tuple_size_v<Tuple_>;
		static_assert(Size > 0, "Can not deal with zero-size tuple.");
		DEBUG_CHECK(index < Size)

		const auto invokeElement__ = [&]<size_t Idx>(std::integral_constant<size_t, Idx>) -> bool
		{
			using El_ = std::add_lvalue_reference_t<std::tuple_element_t<Idx, Tuple_>>;
			static_assert(std::is_invocable_r_v<void, F, El_>, "Tuple accessor can not be invoked for specific element");
			if (Idx != index)
			{
				return false;
			}

			auto& element = std::get<Idx>(tuple_);
			accessor(element);
			return true;
		};

		const auto invokeTuple__ = [&]<size_t ... I>(std::index_sequence<I...>) -> bool
		{
			return (invokeElement__(std::integral_constant<size_t, I>{}) || ...);
		};

		[[maybe_unused]] const bool invoked = invokeTuple__(Tuple::Indexes<Tuple_>{});
		DEBUG_CHECK(invoked)
	}


private:

	
	template<typename T, size_t Index>
	using El = std::remove_cv_t<std::tuple_element_t<Index, T>>;


	//template<typename T, typename Element, size_t ... I>
	//constexpr static bool contains__(std::index_sequence<I...>) {
	//	return (std::is_same_v<Element, El<T, Index>> || ...);
	//}

};



}
