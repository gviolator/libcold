#pragma once

#include "representationbase.h"
#include <cold/meta/classinfo.h>
#include <cold/utils/tupleutility.h>

namespace cold::meta {

template<typename T>
concept ObjectRepresentable = AttributeDefined<T, ClassFieldsAttribute>;

template<typename>
class RepresentedField;

/**
	Represented field
*/
template<typename T, typename ... Attribs>
class RepresentedField<FieldInfo<T, Attribs...>>
{
	using Field = FieldInfo<T, Attribs...>;

public:
	using ValueType = typename Field::ValueType;
	
	template<typename Attribute>
	static inline constexpr bool HasAttribute = Field::template HasAttribute<Attribute>;

	RepresentedField(Field field): m_field(std::move(field))
	{}

	auto value() const
	{
		return represent(m_field.value());
	}

	auto name() const
	{
		return m_field.name();
	}

	decltype(auto) attributes() const
	{
		return m_field.attributes();
	}

	template<typename Attribute>
	constexpr bool hasAttribute() const
	{
		return m_field.template hasAttribute<Attribute>();
	}

private:
	const Field m_field;
};

template<typename T, typename ... Attribs>
RepresentedField(FieldInfo<T, Attribs...> field) -> RepresentedField<FieldInfo<T, Attribs...>>;


template <typename T>
requires requires (const T& field_)
{
	field_.value();
}

using FieldRepresentationType = decltype(std::declval<T>().value());


} // namespace cold::meta

namespace cold::meta_internal {
/**

*/
template<meta::ObjectRepresentable T>
class ObjectRepresentation final : public meta::RepresentationBase<std::remove_const_t<T>, std::remove_const_t<T>>
{
public:
	static constexpr bool IsConst = std::is_const_v<T>;

	ObjectRepresentation(T& instance_): m_instance(&instance_)
	{}

	auto fields() const
	{
		const auto representFields__ = [this] <size_t ... I>(auto&& fields, std::index_sequence<I...>)
		{
			return std::tuple { meta::RepresentedField{std::move(std::get<I>(fields))} ...};
		};

		DEBUG_CHECK(this->m_instance)

		auto fields = meta::classFields(*this->m_instance);
		return representFields__(std::move(fields), Tuple::indexes(fields));
	}

private:

	T* const m_instance;
};

} // namespace cold::meta_internal


namespace cold::meta {

template<ObjectRepresentable T>
using ObjectRepresentation = cold::meta_internal::ObjectRepresentation<T>;

template<ObjectRepresentable T>
using ConstObjectRepresentation = cold::meta_internal::ObjectRepresentation<const T>;


template<ObjectRepresentable T>
ObjectRepresentation<T> represent(T& instance)
{
	return {instance};
}

template<ObjectRepresentable T>
ConstObjectRepresentation<T> represent(const T& instance)
{
	return {instance};
}

} // namespace cold::meta
