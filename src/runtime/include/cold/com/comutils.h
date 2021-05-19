#pragma once
#include <cold/com/ianything.h>
#include <cold/meta/classinfo.h>


namespace cold::com_internal {


template<typename Target, typename T, typename ... Base>
inline bool cast__(T& instance, Target*& target, TypeList<Base...>)
{
	if constexpr (std::is_convertible_v<T*, Target*>)
	{
		target = &static_cast<Target&>(instance);
		return true;
	}
	else
	{
		return (cast__(static_cast<Base&>(instance), target, meta::ClassDirectBase<Base>{}) || ... );
	}
}


template<typename T, typename ... Base>
inline bool rtCast__(T& instance, const std::type_info& targetTypeId, void** target, TypeList<Base...>)
{
	const std::type_info& instanceTypeId = typeid(T);
	if (instanceTypeId == targetTypeId)
	{
		*target = reinterpret_cast<void*>(&instance);
		return true;
	}

	return (rtCast__(static_cast<Base&>(instance), targetTypeId, target, meta::ClassDirectBase<Base>{}) || ... );
}

}


namespace cold::com {


template<typename Target, typename T>
inline Target* cast(T& instance)
{
	using namespace cold::com_internal;

	using Type = std::remove_reference_t<std::remove_const_t<T>>;

	Target* target = nullptr;
	return cast__(const_cast<Type&>(instance), target, meta::ClassDirectBase<Type>{}) ? target : nullptr;
}


template<typename T>
void* rtCast(T& instance, const std::type_info& targetType)
{
	using namespace cold::com_internal;

	using Type = std::remove_reference_t<std::remove_const_t<T>>;

	if (targetType == typeid(IAnything))
	{
		IAnything* anything = com::cast<IAnything>(instance);
		return reinterpret_cast<void*>(anything);
	}
	else if (targetType == typeid(IRefCounted))
	{
		IRefCounted* refCounted = com::cast<IRefCounted>(instance);
		return reinterpret_cast<void*>(refCounted);
	}


	void* target = nullptr;
	return rtCast__(const_cast<Type&>(instance), targetType, &target, meta::ClassDirectBase<Type>{}) ? target : nullptr;
}


template<typename T>
bool rtIs(const std::type_info& targetTypeId)
{
	using Type = std::remove_reference_t<std::remove_const_t<T>>;
	using Bases = meta::ClassAllUniqueBase<Type>;

	if (targetTypeId == typeid(IAnything))
	{
		const auto isAnything__ = [] <typename ... Base> (TypeList<Base...>)
		{
			return (std::is_convertible_v<Base*, IAnything*> || ...);
		};

		return isAnything__ (Bases{});
	}
	else if (targetTypeId == typeid(IRefCounted))
	{
		const auto isRefCounted__ = [] <typename ... Base> (TypeList<Base...>)
		{
			return (std::is_convertible_v<Base*, IRefCounted*> || ...);
		};

		return isRefCounted__ (Bases{});
	}

	const auto rtIs__ = [] <typename ... Base> (const std::type_info& targetTypeId, TypeList<Base...>)
	{
		return ((targetTypeId == typeid(Base)) || ...);
	};

	return rtIs__(targetTypeId, Bases{});
}

}
