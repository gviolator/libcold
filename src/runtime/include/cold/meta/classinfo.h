#pragma once

#include <cold/meta/attribute.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/typelist.h>
#include <cold/utils/tupleutility.h>
#include <cold/utils/preprocessor.h>

#include <string_view>
#include <type_traits>

namespace cold {
namespace meta {


/**
	Class name
*/
struct ClassNameAttribute : Attribute
{
	const char* name;

	ClassNameAttribute(const char* name_): name(name_)
	{}
};


/**

*/
struct ClassBaseAttribute : Attribute
{};

/**

*/
template<typename ... T>
struct ClassBase : AttributeValue<ClassBaseAttribute> {
	using type = TypeList<T...>;
};

/**

*/
template<typename T, typename ... Attribs__>
class FieldInfo
{
public:

	using ValueType = std::remove_cv_t<T>;
	using Attributes = std::tuple<Attribs__...>;

	template<typename A>
	static constexpr inline bool HasAttribute = Tuple::template contains<Attributes, A>();

	FieldInfo(T* value_, std::string_view name_, Attribs__ ... attributes)
		: m_value(value_)
		, m_name(name_)
		, m_attributes{std::move(attributes)...}
	{}

	constexpr bool isConst() const {
		return std::is_const_v<T>;
	}

	T& value() const {
		DEBUG_CHECK(m_value != nullptr, "Attempt to access filed of uninitialized instance")
		return *m_value;
	}

	std::string_view name() const {
		return m_name;
	}

	decltype(auto) attributes() const {
		return (m_attributes);
	}

	template<typename Attribute>
	constexpr bool hasAttribute() const {
		return HasAttribute<Attribute>;
	}

private:

	T* const m_value;
	std::string_view m_name;
	const Attributes m_attributes;
};

template<typename T, typename ... A>
FieldInfo(T*, std::string_view, A...) -> FieldInfo<T, A...>;


/**

*/
struct ClassFieldsAttribute : Attribute
{};

/**

*/
template<typename F>
struct ClassFields : AttributeValue<ClassFieldsAttribute>
{
	F fieldsProvider;

	ClassFields(F fieldsProvider_): fieldsProvider(std::move(fieldsProvider_))
	{}

	template<typename T>
	decltype(auto) operator ()(T* instance) const {
		return fieldsProvider(instance);
	}
};

} // namespace meta

//-----------------------------------------------------------------------------
namespace meta_internal {

/**

*/
template<typename T, bool = attributeDefined<T, meta::ClassBaseAttribute>()>
struct ClassDirectBase__ {
	using type = typename meta::AttributeValueType<T, meta::ClassBaseAttribute>::type;
};

/**

*/
template<typename T>
struct ClassDirectBase__<T, false> {
	using type = TypeList<>;
};


template<typename T, typename Base = typename ClassDirectBase__<T>::type>
struct ClassAllBase__;


template<typename T, typename ... Bases>
struct ClassAllBase__<T, TypeList<Bases...>> {
	using type = typelist::Concat<TypeList<Bases...>, typename ClassAllBase__<Bases>::type...>;
};


template<typename T>
auto classFields__(T* instance) {
	
	using Type = std::remove_cv_t<T>;

	if constexpr (attributeDefined<Type, meta::ClassFieldsAttribute>()) {
		auto fields =  meta::attributeValue<Type, meta::ClassFieldsAttribute>();
		return fields(instance);
	}
	else {
		return std::tuple{};
	}
}

template<typename T, typename = typename ClassDirectBase__<std::remove_const_t<T>>::type>
struct ClassAndBaseFields__;

template<typename T, typename ... Base>
struct ClassAndBaseFields__<T, TypeList<Base...>>
{
	static auto fields(T* instance) {
		return std::tuple_cat(
			ClassAndBaseFields__<Base>::fields(static_cast<Base*>(instance)) ... ,
			classFields__(instance)
		);
	}
};



} // namespace meta_internal

//-----------------------------------------------------------------------------
namespace meta {

template<typename T>
using ClassDirectBase = typename cold::meta_internal::ClassDirectBase__<T>::type;

template<typename T>
using ClassAllBase = typename cold::meta_internal::ClassAllBase__<T>::type;

template<typename T>
using ClassAllUniqueBase = typelist::Distinct<ClassAllBase<T>>;

template<typename T>
[[nodiscard]] auto classFields(T& instance) { // T maybe const T
	return cold::meta_internal::ClassAndBaseFields__<T>::fields(&instance);
}

template<typename T>
[[nodiscard]] auto classFields() {
	return cold::meta_internal::ClassAndBaseFields__<T>::fields(nullptr);
}


//template<typename ?


} // namespace meta
} // namespace cold

//-----------------------------------------------------------------------------

#define CLASS_INFO(...)\
CLASS_ATTRIBUTES \
	__VA_ARGS__ \
END_CLASS_ATTRIBUTES


#define CLASS_BASE(...) cold::meta::ClassBase<__VA_ARGS__>{}

#define DECLARE_CLASS_BASE(...) \
CLASS_ATTRIBUTES\
	CLASS_BASE(__VA_ARGS__)\
END_CLASS_ATTRIBUTES


//#define CLASS_NAMED_FIELD(field, name, ...) cold::meta::FieldInfo{ (instance ? &instance->## field : nullptr), #field, __VA_ARGS__}
#define CLASS_NAMED_FIELD(field, name, ...) cold::meta::FieldInfo{instance == nullptr ? nullptr : &instance-> field, name, __VA_ARGS__}

#define CLASS_FIELD(field,...) CLASS_NAMED_FIELD(field, #field, __VA_ARGS__)

#define CLASS_FIELDS(...) cold::meta::ClassFields{[]<typename T>(T* instance) { \
	return std::tuple{__VA_ARGS__}; \
}} \


// #define CLASS_FIELDS(...) cold::meta::ClassFields([](auto instance) { \
// 	DECLARE_CLASS_FIELDS(__VA_ARGS__) \
// }) \
// \

// template<typename ... F>
// auto CLASS_FIELDS(F ... fields) {
// 	return cold::meta::ClassFields([](auto instance) {
// 		return std::tuple {fields ...};
// 	}
// }