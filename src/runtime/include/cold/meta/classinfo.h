#pragma once

#include <cold/meta/attribute.h>
#include <cold/meta/functioninfo.h>
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
	const char* const name;

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

	constexpr bool isConst() const
	{
		return std::is_const_v<T>;
	}

	T& value() const
	{
		DEBUG_CHECK(m_value != nullptr, "Attempt to access filed of uninitialized instance")
		return *m_value;
	}

	std::string_view name() const
	{
		return m_name;
	}

	decltype(auto) attributes() const
	{
		return (m_attributes);
	}

	template<typename Attribute>
	constexpr bool hasAttribute() const
	{
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



/// <summary>
///
/// </summary>
template<auto F, typename ... Attributes>
class MethodInfo
{
public:
	using FunctionType = decltype(F);
	using FunctionTypeInfo = meta::FunctionTypeInfo<FunctionType>;
	using AttributesTuple = std::tuple<Attributes...>;

	inline constexpr static bool IsMemberFunction = std::is_member_function_pointer_v<FunctionType>; 

	inline constexpr static FunctionType Func = F;

	MethodInfo(std::string_view name_, Attributes ... attributes_)
		: m_name(name_)
		, m_attributes{std::move(attributes_)...}
	{}

	std::string_view name() const
	{
		return m_name;
	}

	const AttributesTuple& attributes() const
	{
		return m_attributes;
	}

private:

	std::string_view m_name;
	AttributesTuple m_attributes;
};


template<auto F, typename ... Attributes>
auto createMethodInfo(std::string_view name, Attributes&& ... attributes)
{
	return MethodInfo<F, Attributes...>{name, std::forward<Attributes>(attributes)...};
}



/// <summary>
///
/// </summary>
struct ClassMethodsAttribute final : meta::Attribute
{};


/// <summary>
///
/// </summary>
template<typename ...M>
class ClassMethods final : public meta::AttributeValue<ClassMethodsAttribute>
{
public:

	ClassMethods(M... methods_) : m_methods {std::move(methods_)...}
	{}

	const auto methods() const
	{
		return m_methods;
	}

private:

	std::tuple<M...> m_methods;
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

	if constexpr (attributeDefined<Type, meta::ClassFieldsAttribute>())
	{
		auto fields =  meta::attributeValue<Type, meta::ClassFieldsAttribute>();
		return fields(instance);
	}
	else
	{
		return std::tuple{};
	}
}

template<typename T, typename = typename ClassDirectBase__<std::remove_const_t<T>>::type>
struct ClassAndBaseFields__;

template<typename T, typename ... Base>
struct ClassAndBaseFields__<T, TypeList<Base...>>
{
	static auto fields(T* instance)
	{
		return std::tuple_cat(
			ClassAndBaseFields__<Base>::fields(static_cast<Base*>(instance)) ... ,
			classFields__(instance)
		);
	}
};

#pragma region Class meta methods access

template<typename T>
auto classMethods__()
{
	using Type = std::remove_cv_t<T>;

	if constexpr (!meta::AttributeDefined<Type, meta::ClassMethodsAttribute>)
	{
		return std::tuple{};
	}
	else
	{
		return meta::attributeValue<Type, meta::ClassMethodsAttribute>().methods();
	}
}

template<typename T, typename = typename ClassDirectBase__<std::remove_const_t<T>>::type>
struct ClassAndBaseMethods;

template<typename T, typename ... Base>
struct ClassAndBaseMethods<T, TypeList<Base...>>
{
	static auto methods()
	{
		return std::tuple_cat(
			ClassAndBaseMethods<Base>::methods() ...,
			classMethods__<T>()
		);
	}
};

#pragma endregion



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
[[nodiscard]] auto classFields(T& instance)  // T maybe const T
{
	return cold::meta_internal::ClassAndBaseFields__<T>::fields(&instance);
}

template<typename T>
[[nodiscard]] auto classFields()
{
	return cold::meta_internal::ClassAndBaseFields__<T>::fields(nullptr);
}

template<typename T>
[[nodiscard]] decltype(auto) classMethods()
{
	return cold::meta_internal::ClassAndBaseMethods<T>::methods();
}

namespace meta_internal 
{

template<auto Target, typename F>
constexpr bool sameMethod()
{
	if constexpr (std::is_convertible_v<typename F::FunctionType, decltype(Target)>)
	{
		return F::Func == Target;
	}

	return false;
}


template<auto Target, int Idx, typename>
struct MethodInfoFinder;


template<auto Target, int Idx, typename Head, typename ... Tail>
struct MethodInfoFinder<Target, Idx, std::tuple<Head, Tail...>>
{
	static constexpr inline int result = sameMethod<Target, Head>() ? Idx : MethodInfoFinder<Target, Idx + 1, std::tuple<Tail...>>::result;
};

template<auto Target, int Idx>
struct MethodInfoFinder<Target, Idx, std::tuple<>>
{
	static constexpr inline int result = -1;
};

} // namespace meta_internal 


template<auto Func>
static auto findClassMethodInfo()
{
	using FuncType = decltype(Func);

	static_assert(IsMemberFunction<FuncType>, "Only member function can be used");

	using FuncInfo = FunctionTypeInfo<FuncType>;

	auto methods = classMethods<FuncInfo::Class>();
	
	constexpr int index = meta_internal::MethodInfoFinder<Func, 0,  decltype(methods)>::result;

	if constexpr (index < 0)
	{
		return std::false_type {};
	}
	else
	{
		return std::get<index>(methods);
	}
}



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


#if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL

#define CLASS_METHOD(Class_, method, ... ) cold::meta::createMethodInfo<&Class_::method>(#method __VA_ARGS__ )
#define CLASS_NAMED_METHOD(class_, method, name, ...) cold::meta::createMethodInfo<&class_::method>(name, __VA_ARGS__)

#else

#define CLASS_METHOD(Class_, method, ... ) cold::meta::createMethodInfo<&Class_::method>(#method __VA_OPT__(,) __VA_ARGS__ )
#define CLASS_NAMED_METHOD(class_, method, name, ...) cold::meta::createMethodInfo<&class_::method>(name, __VA_OPT__(,) __VA_ARGS__ )


#endif


#define CLASS_METHODS(...) cold::meta::ClassMethods { __VA_ARGS__ }
