#pragma once

#include <cold/meta/classinfo.h>
#include <cold/com/comclass.h>


namespace cold::proxy {

namespace proxy_internal {


//
template<typename T>
constexpr static T defaultResult(TypeList<T>)
{
	return T{};
}


constexpr static void defaultResult(TypeList<void>)
{}


} //namespace meta_internal


/// <summary>
///
/// </summary>
struct ProxyImplementationAttribute : meta::Attribute
{};


/// <summary>
///
/// </summary>
template<template <typename> class Proxy>
struct ProxyImplementation : meta::AttributeValue<ProxyImplementationAttribute>
{
	template<typename ProxyClass_>
	using type = Proxy<ProxyClass_>;
};


template<typename ProxyClass_, typename ... T>
struct ProxyClassInheritance : public meta::AttributeValueType<T, ProxyImplementationAttribute>::template type<ProxyClass_> ... 
{};


template<auto Func, typename ProxyClass_, typename ... P>
decltype(auto) proxyInvoke(const ProxyClass_& proxy, P&& ... p) noexcept(meta::FunctionTypeInfo<decltype(Func)>::NoExcept)
{
	auto method = meta::findClassMethodInfo<Func>();

	static_assert(!std::is_same_v<decltype(method), std::false_type>);

	if constexpr (std::is_same_v<decltype(method), std::false_type>)
	{
		/*using FuncInfo = FunctionTypeInfo<decltype(Func)>;

		using Class_ = typename FuncInfo::Class;

		const auto errorMessage = cold::strfmt("Interface [%1] does not expose method meta information [%2]", typeid(Class_), typeid(Func));

		throw std::exception(errorMessage.c_str());

		return meta_internal::defaultResult(TypeList<typename FuncInfo::Result>{});*/
	}
	else
	{
		using Marshal = typename ProxyClass_::Marshal;

		auto& nonConstProxy = const_cast<ProxyClass_&>(proxy);

		return Marshal::invoke(nonConstProxy, std::move(method), std::forward<P>(p) ...);
	}
}

#ifdef _MSC_VER
#pragma warning(push)
// #pragma warning(disable: WARN_INHERITS_VIA_DOMINANCE)
#endif


template<typename Marshal_, typename ... Itfs>
class ProxyComClass final
	: public Marshal_::template ProxyBase<Itfs...>
	, public ProxyClassInheritance<ProxyComClass<Marshal_, Itfs...>, Itfs ...>
{
	COMCLASS_(Marshal_::template ProxyBase<Itfs...>,  Itfs...)

public:
	using Marshal = Marshal_;

	template<typename ... P>
	ProxyComClass(P&&...parameters) : Marshal::template ProxyBase<Itfs...>(std::forward<P>(parameters)...)
	{}
};


/**
* Same as ProxyComClass, but forcing to inherit IRefCounted interface.
* This class used by system in cases when created proxy consist of interfaces that does not provide required IRefCounted.
*/
template<typename Marshal_, typename ... Itfs>
class ProxyComClassWithRefCounted final
	: public Marshal_::template ProxyBase<Itfs...>
	, public virtual IRefCounted
	, public ProxyClassInheritance<ProxyComClass<Marshal_, Itfs...>, Itfs ...>
{
	COMCLASS_(Marshal_::template ProxyBase<Itfs...>,  Itfs...)

public:
	using Marshal = Marshal_;

	template<typename ... P>
	ProxyComClassWithRefCounted(P&&...parameters) : Marshal::template ProxyBase<Itfs...>(std::forward<P>(parameters)...)
	{}
};


/// <summary>
///
/// </summary>
template<typename Marshal_, typename ...Itfs>
class Proxy
	: public Marshal_::template ProxyBase<Itfs...>
	, public ProxyClassInheritance<Proxy<Marshal_, Itfs...>, Itfs ...>
{
	CLASS_INFO(
		CLASS_BASE(Marshal_::template ProxyBase<Itfs...>, Itfs...)
	)

public:

	using Marshal = Marshal_;

	template<typename ... Args>
	Proxy(Args&&...args) : Marshal::template ProxyBase<Itfs...>(std::forward<Args>(args)...)
	{}
};


#ifdef _MSC_VER
#pragma warning(pop)
#endif

//-----------------------------------------------------------------------------
/// <summary>
///
/// </summary>
template<typename Marshal, typename Interface, typename ... AddInterfaces, typename ... P>
ComPtr<Interface> constructProxyComClass(P&& ... p)
{
	static_assert(meta::AttributeDefined<Interface, ProxyImplementationAttribute>, "Proxy not defined for target interface, use ProxyImplementationAttribute");
	static_assert((meta::AttributeDefined<AddInterfaces, ProxyImplementationAttribute> && ...), "Proxy not defined for some added interfaces , use ProxyImplementationAttribute");

	constexpr bool HasRefCountedObject = std::is_base_of_v<IRefCounted, Interface> || (std::is_base_of_v<IRefCounted, AddInterfaces> || ...);

	if constexpr (HasRefCountedObject)
	{
		using ProxyClass = ProxyComClass<Marshal, Interface, AddInterfaces...>;
		return com::createInstance<ProxyClass, Interface>(std::forward<P>(p)...);
	}
	else
	{
		using ProxyClass = ProxyComClassWithRefCounted<Marshal, Interface, AddInterfaces...>;
		return com::createInstance<ProxyClass, Interface>(std::forward<P>(p)...);
	}
}


/// <summary>
///
/// </summary>
template<typename Marshal, typename Interface, typename ... AddInterfaces, typename ... P>
ComPtr<Interface> constructProxyComClass_(TypeList<Interface, AddInterfaces...>, P&& ... p)
{
	return constructProxyComClass<Marshal, Interface, AddInterfaces...>(std::forward<P>(p)...);
}


/// <summary>
///
/// </summary>
template<typename Marshal, typename Interface, typename ... AddInterfaces, typename ... P>
auto constructProxy(P&& ... p)
{
	static_assert(meta::AttributeDefined<Interface, ProxyImplementationAttribute>, "Proxy not defined for target interface, use ProxyImplementationAttribute");

	static_assert((meta::AttributeDefined<AddInterfaces, ProxyImplementationAttribute> && ...), "Proxy not defined for some added interfaces , use ProxyImplementationAttribute");

	constexpr bool HasRefCountedObject = std::is_base_of_v<IRefCounted, Interface> || (std::is_base_of_v<IRefCounted, AddInterfaces> || ...);

	if constexpr (HasRefCountedObject)
	{
		return constructProxyComClass<Marshal, Interface, AddInterfaces...>(std::forward<P>(p)...);
	}
	else
	{
		using ProxyClass = Proxy<Marshal, Interface, AddInterfaces...>;

		static_assert(std::has_virtual_destructor_v<Interface>, "Type must have virtual destructor.");

		std::unique_ptr<Interface> proxy = std::make_unique<ProxyClass>(std::forward<P>(p)...);

		return proxy;
	}
}



} 


#define PROXY_FORWARD(Proxy_) template <typename> class Proxy_ ;


#define PROXY(ProxyImplementationClass_, Interface_) \
template<typename ProxyClass__>\
class ProxyImplementationClass_ : public virtual Interface_\



#define PROXY_INVOKE(Member, ...) cold::proxy::proxyInvoke<&Member>(static_cast<const ProxyClass__&>(*this), __VA_ARGS__)


#define CLASS_PROXY(Proxy_) \
	cold::proxy::ProxyImplementation<Proxy_>{}


#define DECLARE_PROXY(Interface, Proxy_)\
ATTRIBUTE_VALUE(Interface, cold::proxy::ProxyImplementationAttribute, cold::proxy::ProxyImplementation<Proxy_>{})

