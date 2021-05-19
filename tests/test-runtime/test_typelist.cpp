#include "pch.h"
#include <cold/utils/typelist.h>
#include <cold/utils/typelist/contains.h>


using namespace cold;

namespace {

template<typename T>
struct MakeOptional {
	using type = std::optional<T>;
};


}
//
//
//template<typename T>
//concept IsTypeList = true;
//
//
//template<template <typename> class F>
//concept MetaFunc = requires
//{
//	typename F<int>::type;
//};
//
//
//
//template<template <typename > class F, typename T>
//struct SampleMap
//{
//	using type = F<T>;
//};
//
//
//template<template <typename > class F, typename T>
//requires MetaFunc<F>
//struct SampleMap<F, T>
//{
//	using type = typename F<T>::type;
//};
//


TEST(Common_TypeList, Transform) {
	using InitialTypeList = TypeList<int, unsigned, short>;
	using OptionalsTypeList = TypeList<std::optional<int>, std::optional<unsigned>, std::optional<short>>;
	using TypeList2 = typelist::Transform<InitialTypeList, MakeOptional>;
	using TypeList3 = typelist::Transform<InitialTypeList, std::optional>;

	static_assert(std::is_same_v<TypeList2, OptionalsTypeList>);
	static_assert(std::is_same_v<TypeList2, TypeList3>);
}

TEST(Common_TypeList, Concat) {
	static_assert(std::is_same_v<typelist::Concat<TypeList<>>, TypeList<>>);
	static_assert(std::is_same_v<typelist::Concat<TypeList<>, TypeList<>>, TypeList<>>);
	static_assert(std::is_same_v<typelist::Concat<TypeList<int>, TypeList<float>>, TypeList<int, float>>);
	static_assert(std::is_same_v<typelist::Concat<TypeList<unsigned, short>, TypeList<float, double>, TypeList<long>>, TypeList<unsigned, short, float, double, long>>);
}

TEST(Common_TypeList, Distinct) {
	static_assert(std::is_same_v<typelist::Distinct<TypeList<>>, TypeList<>>);
	static_assert(std::is_same_v<typelist::Distinct<TypeList<int>>, TypeList<int>>);
	static_assert(std::is_same_v<typelist::Distinct<TypeList<int, int, int, int>>, TypeList<int>>);
	static_assert(std::is_same_v<typelist::Distinct<TypeList<int, float, int, float>>, TypeList<int, float>>);
}

TEST(Common_TypeList, Contains) {
	using IntsList = TypeList<int, unsigned, short>;

	static_assert(typelist::Contains<IntsList, unsigned>);
	static_assert(typelist::Contains<IntsList, int>);
	static_assert(typelist::Contains<IntsList, short>);
	static_assert(!typelist::Contains<IntsList, float>);
	static_assert(!typelist::Contains<TypeList<>, float>);
}

TEST(Common_TypeList, ContainsAll) {

	using IntsList = TypeList<short, int, long, unsigned short, unsigned, unsigned long>;
	
	static_assert(typelist::ContainsAll<IntsList, TypeList<int, unsigned>>);
	static_assert(typelist::ContainsAll<IntsList, TypeList<>>);
	static_assert(!typelist::ContainsAll<IntsList, TypeList<unsigned, double>>);
	static_assert(!typelist::ContainsAll<IntsList, TypeList<float>>);

	static_assert(!typelist::ContainsAll<TypeList<unsigned>, TypeList<unsigned, float>>);
	static_assert(typelist::ContainsAll<TypeList<unsigned, float>, TypeList<unsigned>>);
	static_assert(typelist::ContainsAll<TypeList<unsigned>, TypeList<unsigned, unsigned>>);
}