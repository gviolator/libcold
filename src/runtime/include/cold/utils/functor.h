#pragma once

#include <cold/com/interface.h>
#include <cold/meta/functioninfo.h>

#include <memory.h>

namespace cold {

template<bool NoExcept_, typename Result_, typename ParametersList_>
struct FunctorClass;


/// <summary>
///
/// </summary>
template<bool NoExcept_, typename Result_, typename ... Parameters_>
struct INTERFACE_API FunctorClass<NoExcept_, Result_, TypeList<Parameters_...>>
{
	using UniquePtr = std::unique_ptr<FunctorClass>;

	template<typename F>
	[[nodiscard]] static UniquePtr makeUnique(F f);

	virtual ~FunctorClass() = default;

	virtual Result_ invoke(Parameters_...) noexcept(NoExcept_) = 0;

	template<typename ... U>
	decltype(auto) operator() (U&&...p) noexcept(NoExcept_)
	{
		return invoke(std::forward<U>(p)...);
	}
};


/// <summary>
///
/// </summary>
template<typename Callable_, bool NoExcept_, typename Result_, typename ... Parameters_>
class DefaultFunctor: public FunctorClass<NoExcept_, Result_, TypeList<Parameters_...>>
{
public:

	using Callable = Callable_;

	DefaultFunctor(Callable_ callable_): m_callable(std::move(callable_))
	{
		static_assert(std::is_invocable_r_v<Result_, Callable_, Parameters_...>, "Invalid functor argument");
		if constexpr (NoExcept_)
		{
			static_assert(noexcept(callable_(std::declval<Parameters_>()...)), "Functor must specify noexcept specification");
		}
	}

	virtual Result_ invoke(Parameters_... p) noexcept(NoExcept_) override
	{
		return const_cast<Callable&>(m_callable)(std::forward<Parameters_>(p)...);
	}

private:

	const Callable m_callable;
};


template<typename Callable_>
using Functor = FunctorClass<
	meta::FunctionTypeInfo<Callable_>::NoExcept,
	typename meta::FunctionTypeInfo<Callable_>::Result,
	typename meta::FunctionTypeInfo<Callable_>::ParametersList>;



template<bool NoExcept_, typename Result_, typename ... Parameters_>
template<typename Callable_>
auto FunctorClass<NoExcept_, Result_, TypeList<Parameters_...>>::makeUnique(Callable_ callable_) -> typename FunctorClass::UniquePtr
{
	static_assert(std::is_invocable_r_v<Result_, Callable_, Parameters_...>, "Functor has unacceptable parameters");

	if constexpr (NoExcept_)
	{
		static_assert(noexcept(callable_(std::declval<Parameters_>()...)), "Functor must specify noexcept specification");
	}

	using FunctorClass__ = DefaultFunctor<Callable_, NoExcept_, Result_, Parameters_...>;

#if !defined(_NDEBUG)
	using CallableInfo = meta::FunctionTypeInfo<Callable_>;

	[[maybe_unused]] constexpr bool RequireProxy =
		NoExcept_ != CallableInfo::NoExcept ||
		!std::is_same_v<TypeList<Parameters_...>, typename CallableInfo::ParametersList>;

	//if constexpr (RequireProxy) {

	//	auto proxy = [callable = std::move(callable_)](Parameters_ ... params) noexcept(NoExcept_) -> Result_ {
	//		return callable(std::forward<Parameters_>(params)...);
	//	};

#endif

	return std::make_unique<FunctorClass__>(std::move(callable_));
}

}
