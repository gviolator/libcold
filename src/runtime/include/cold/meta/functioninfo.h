#pragma once
#include <cold/utils/typelist.h>

#include <type_traits>

namespace cold::meta {
//
//template<typename F>
//concept MemberFunction = std::is_member_function_pointer_v<F>;



namespace meta_internal {

template<typename F>
decltype(&F::operator(), std::true_type{}) isFunctorHelper(int);

template<typename F>
std::false_type isFunctorHelper(...);

}


template<typename F>
inline constexpr bool IsMemberFunction = std::is_member_function_pointer_v<F>;

template<typename F>
inline constexpr bool IsFunctor = decltype(meta_internal::isFunctorHelper<F>(int{}))::value;


namespace meta_internal {


/// <summary>
///
/// </summary>
template<bool Const_, bool NoExcept_, typename R, typename ... P>
struct FunctionTypeInfoBase {

	using Result = R;
	using ParametersList = TypeList<P...>;

	inline constexpr static bool Const = Const_;
	inline constexpr static bool NoExcept = NoExcept_;
};


/// <summary>
///
/// </summary>
template<bool Const_, bool NoExcept, typename Class_, typename R, typename ... P>
struct MemberFunctionTypeInfoBase : FunctionTypeInfoBase<Const_, NoExcept, R, P...> {
	using Class = Class_;
};



template<typename F>
struct FunctionTypeInfo;

// Functor
template<typename R, typename ... P>
struct FunctionTypeInfo<R (P...)> : FunctionTypeInfoBase<false, false, R, P...>
{};


template<typename R, typename ... P>
struct FunctionTypeInfo<R (P...) noexcept> : FunctionTypeInfoBase<false, true, R, P...>
{};


template<typename R, typename ... P>
struct FunctionTypeInfo<R (P...) const> : FunctionTypeInfoBase<true, false, R, P...>
{};


template<typename R, typename ... P>
struct FunctionTypeInfo<R (P...) const noexcept> : FunctionTypeInfoBase<true, true, R, P...>
{};



// Function pointer
template<typename R, typename ... P>
struct FunctionTypeInfo<R (*) (P...)> : FunctionTypeInfoBase<false, false, R, P...>
{};


template<typename R, typename ... P>
struct FunctionTypeInfo<R (*) (P...) noexcept> : FunctionTypeInfoBase<false, true, R, P...>
{};


// Member function pointer
// non const instance
template<typename Class_, typename R,  typename ... P>
struct FunctionTypeInfo<R (Class_::*)(P...)> : MemberFunctionTypeInfoBase<false, false, Class_, R, P...>
{};


template<typename Class_, typename R,  typename ... P>
struct FunctionTypeInfo<R (Class_::*)(P...) noexcept> : MemberFunctionTypeInfoBase<false, true, Class_, R, P...>
{};


template<typename Class_, typename R,  typename ... P>
struct FunctionTypeInfo<R (Class_::*)(P...) const> : MemberFunctionTypeInfoBase<true, false, Class_, R, P...>
{};


template<typename Class_, typename R,  typename ... P>
struct FunctionTypeInfo<R (Class_::*)(P...) const noexcept> : meta_internal::MemberFunctionTypeInfoBase<true, true, Class_, R, P...>
{};


// const instance
template<typename Class_, typename R,  typename ... P>
struct FunctionTypeInfo<R (Class_::* const)(P...)> : meta_internal::MemberFunctionTypeInfoBase<false, false, Class_, R, P...>
{};


template<typename Class_, typename R,  typename ... P>
struct FunctionTypeInfo<R (Class_::* const)(P...) noexcept> : meta_internal::MemberFunctionTypeInfoBase<false, true, Class_, R, P...>
{};


template<typename Class_, typename R,  typename ... P>
struct FunctionTypeInfo<R (Class_::* const)(P...) const> : meta_internal::MemberFunctionTypeInfoBase<true, false, Class_, R, P...>
{};


template<typename Class_, typename R,  typename ... P>
struct FunctionTypeInfo<R (Class_::* const)(P...) const noexcept> : meta_internal::MemberFunctionTypeInfoBase<true, true, Class_, R, P...>
{};


template<typename F, bool = IsFunctor<F>>
struct GetFunctionTypeInfo
{
	using type = FunctionTypeInfo<decltype(&F::operator())>;
};

template<typename F>
struct GetFunctionTypeInfo<F, false>
{
	using type = FunctionTypeInfo<F>;
};


} // meta_internal



template<typename F>
using FunctionTypeInfo = typename meta_internal::GetFunctionTypeInfo<std::decay_t<F>>::type;


}
