#pragma once
#include <cold/utils/typelist/contains.h>
#include <cold/utils/typelist/transform.h>
#include <cold/meta/concepts.h>


namespace cold {
namespace meta {

/**
*/
struct Attribute
{};


/**
*/
template<typename A>
struct AttributeValue {
	using AttributeT = A;
};

} // namespace meta

//-----------------------------------------------------------------------------
namespace meta_internal {


/// <summary>
///
/// </summary>
struct ClassAttributesStorage
{};


/// <summary>
///
/// </summary>
template<typename T>
concept ClassAttributesProvider = requires
{
	requires std::is_base_of_v<ClassAttributesStorage, typename T::ClassAttributes__>;
	{ T::ClassAttributes__::attributes() } -> concepts::TemplateOf<std::tuple>;
};


template<ClassAttributesProvider T>
auto classAttributes()
{
	return T::ClassAttributes__::attributes();
}


template<typename T>
std::tuple<> classAttributes()
{
	return {};
}


template<typename T, typename A>
struct ExternalAttribute
{
	inline constexpr static bool Defined = false;
};


template<typename T>
constexpr decltype(std::declval<typename T::AttributeT>(), bool{}) isAttributeValue__(int) { return true; }

template<typename T>
constexpr inline bool isAttributeValue__(...) { return false; }

#if 0

template<typename T>
requires requires {typename T::AttributeT;}
constexpr bool IsAttributeValue__() { return true; }

template<typename T>
constexpr bool IsAttributeValue__() { return false; }

#endif


template<typename A, bool = isAttributeValue__<A>(0)>
struct UnwrapAttribute__ {
	using type = typename A::AttributeT;
};

template<typename A>
struct UnwrapAttribute__<A, false> {
	using type = A;
};


// //#ifdef __clang__

 template<typename T>
 using UnwrapAttribute = UnwrapAttribute__<T>;

// //#else
// //#endif



template<typename T, typename A>
/*consteval */constexpr bool attributeDefined()
{
	if constexpr (ExternalAttribute<T, A>::Defined) {
		return true;
	}
	else {
		using ClassSpecifiedAttributes = typelist::TypeListOf<decltype(classAttributes<T>())>;
		using AttributeTypes = typelist::Transform<ClassSpecifiedAttributes, UnwrapAttribute>;

		return typelist::Contains<AttributeTypes, A>;
	}
}

} // namespace meta_internal

//-----------------------------------------------------------------------------
namespace meta {

/// <summary>
///
/// </summary>
template<typename T, typename A>
inline constexpr bool AttributeDefined = cold::meta_internal::attributeDefined<T,A>();

/// <summary>
///
/// </summary>
template<typename T, typename A>
auto attributeValue()
{
	using namespace cold::meta_internal;

	if constexpr (ExternalAttribute<T, A>::Defined) {
		return ExternalAttribute<T, A>::value();
	}
	else
	{
		auto attributes = classAttributes<T>();

		using ClassSpecifiedAttributes = typelist::TypeListOf<decltype(attributes)>;
		using Attributes = typelist::Transform<ClassSpecifiedAttributes, UnwrapAttribute>;

		if constexpr (constexpr int AttributeIndex = typelist::ElementIndex<Attributes, A>; AttributeIndex >= 0) {
			return std::get<AttributeIndex>(attributes);
		}
		else {
			static_assert(AttributeIndex >= 0, "Requested attribute not defined for specified type");

			return 0;
		}
	}
}

/// <summary>
/// 
/// </summary>
template<typename T, typename A>
using AttributeValueType = decltype(attributeValue<T, A>());

} // namespace meta
} // naemspace cold


#define CLASS_ATTRIBUTES \
public:\
\
struct ClassAttributes__ final : cold::meta_internal::ClassAttributesStorage\
{\
	static auto attributes() \
	{ \
		return std::tuple {


#define END_CLASS_ATTRIBUTES \
		}; \
	} \
\
};
