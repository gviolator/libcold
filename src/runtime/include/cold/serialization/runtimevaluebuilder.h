#pragma once
#include <cold/serialization/runtimevalue.h>
#include <cold/serialization/valuerepresentation.h>
#include <cold/serialization/valuerepresentation/arrayrepresentation.h>
#include <cold/serialization/valuerepresentation/optionalrepresentation.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/result.h>
#include <cold/utils/typeutility.h>
#include <cold/com/comclass.h>

namespace cold {

namespace cold_internal {

template<typename T>
class RuntimePrimitive;

template<typename T>
class RuntimeOptional;

template<typename>
class RuntimeArray;

template<typename T>
class RuntimeObject;


} // namespace cold_internal

// objects
template<meta::ObjectRepresentable T>
ComPtr<cold_internal::RuntimeObject<T>> runtimeValue(T&);

template<meta::ObjectRepresentable T>
ComPtr<cold_internal::RuntimeObject<const T>> runtimeValue(const T&);

// arrays
template<meta::ArrayRepresentable T>
ComPtr<cold_internal::RuntimeArray<T>> runtimeValue(T&);

template<meta::ArrayRepresentable T>
ComPtr<cold_internal::RuntimeArray<const T>> runtimeValue(const T&);

// optionals
template<meta::OptionalRepresentable T>
ComPtr<cold_internal::RuntimeOptional<T>> runtimeValue(T&);

template<meta::OptionalRepresentable T>
ComPtr<cold_internal::RuntimeOptional<const T>> runtimeValue(const T&);

// primitives
template<typename T>
ComPtr<cold_internal::RuntimePrimitive<T>> runtimeValue(T&);

template<typename T>
ComPtr<cold_internal::RuntimePrimitive<const T>> runtimeValue(const T&);



namespace cold_internal {

template<typename T>
using RuntimeValueTypeOf = decltype(cold::runtimeValue(std::declval<T>()));


/**
*/
template<typename T>
class RuntimePrimitive final : public TypedRuntimePrimitiveValue<meta::PrimitiveRepresentationValueType<std::remove_const_t<T>>>
{
	COMCLASS_(TypedRuntimePrimitiveValue<meta::PrimitiveRepresentationValueType<std::remove_const_t<T>>>)

	using ValueType = meta::PrimitiveRepresentationValueType<std::remove_const_t<T>>;

public:

	RuntimePrimitive(T& value_): m_value(value_)
	{}

	ValueType get() const override {
		using namespace cold::meta;

		return getPrimitiveValue(m_value);
	}

	bool isMutable() const override {
		return !std::is_const_v<T>;
	}

	Result<> set([[maybe_unused]] ValueType value) override {
		using namespace cold::meta;

		if constexpr (!std::is_const_v<T>) {
			using ResultType = decltype(setPrimitiveValue(m_value, std::move(value)));

			if constexpr (std::is_same_v<ResultType, void>) {
				setPrimitiveValue(m_value, std::move(value));
			}
			else {
				static_assert(IsResult<ResultType>, "Expected Result<> type");
				return setPrimitiveValue(m_value, std::move(value));
			}
		}
		else {
			RUNTIME_FAILURE("Attempt to modify constant value")
		}

		return success;
	}

private:
	T& m_value;
};

/**
*/
template<typename T>
class RuntimeOptional final : public RuntimeOptionalValue
{
	using Optional = std::remove_const_t<T>;
	using ValueType = std::conditional_t<
		std::is_const_v<T>,
		std::add_const_t<typename meta::OptionalValueOperations<Optional>::ValueType>,
		typename meta::OptionalValueOperations<Optional>::ValueType>
		;

	COMCLASS_(RuntimeOptionalValue)

public:

	RuntimeOptional(T& opt_): m_optional(opt_)
	{}

	bool isMutable() const override {
		return !std::is_const_v<T>;
	}

	bool hasValue() const override {
		return meta::OptionalValueOperations<Optional>::hasValue(m_optional);
	}

	const RuntimeValue::Ptr value() const override {
		DEBUG_CHECK(this->hasValue(), "Optional has no value")

		auto value = m_wrappedValue ?
			m_wrappedValue :
			m_wrappedValue.emplace(runtimeValue(meta::OptionalValueOperations<Optional>::value(m_optional)));

		return *value;
	}

	RuntimeValue::Ptr value() override {
		DEBUG_CHECK(this->isMutable())
		DEBUG_CHECK(this->hasValue(), "Optional has no value")

		auto value = m_wrappedValue ?
			m_wrappedValue :
			m_wrappedValue.emplace(runtimeValue(meta::OptionalValueOperations<Optional>::value(m_optional)));

		return *value;

	}

	void reset() override {
		DEBUG_CHECK(this->isMutable())
		m_wrappedValue.reset();
		meta::OptionalValueOperations<Optional>::reset(m_optional);
	}

	RuntimeValue::Ptr emplace(RuntimeValue::Ptr value = cold::nothing) override {
		DEBUG_CHECK(this->isMutable())
		decltype(auto) valueRef = meta::OptionalValueOperations<Optional>::emplace(m_optional);
		static_assert(std::is_lvalue_reference_v<decltype(valueRef)>);
		auto thisVal = m_wrappedValue.emplace(runtimeValue(meta::OptionalValueOperations<Optional>::value(valueRef)));

		if (value)
		{
			[[maybe_unused]] auto res = RuntimeValue::assign(thisVal, value);
		}
		return thisVal;
	}

private:
	using RuntimeValueType = RuntimeValueTypeOf<ValueType>;

	T& m_optional;
	mutable std::optional<RuntimeValueType> m_wrappedValue;
};


/**
*/
template<typename T>
class RuntimeArray final : public RuntimeArrayValue
{
	COMCLASS_(RuntimeArrayValue)

	using Array = std::remove_const_t<T>;
	using Element =  decltype(runtimeValue(meta::ArrayValueOperations<Array>::element(lvalueRef<T>(), 0)));
	static constexpr bool IsMutable = !std::is_const_v<T>;

public:

	RuntimeArray(T& array_): m_array(array_) {
		using namespace cold::meta;

		m_elements.reserve(ArrayValueOperations<Array>::size(m_array));
		for (auto& element : m_array) {
			[[maybe_unused]]Element& el = m_elements.emplace_back(runtimeValue(element));
			DEBUG_CHECK(el->isMutable() == IsMutable)
		}
	}

	bool isMutable() const override {
		return IsMutable;
	}

	size_t size() const override {
		return meta::ArrayValueOperations<Array>::size(m_array);
	}

	const RuntimeValue::Ptr element(size_t index) const override {
		DEBUG_CHECK(m_elements.size() == m_array.size() && index <= m_elements.size())
		//ComPtr<const RuntimeValue> elem = cold::com::createInstance<const Element>(m_elements[index]);

		return m_elements[index];
	}

	RuntimeValue::Ptr element(size_t index) override {
		DEBUG_CHECK(m_elements.size() == m_array.size() && index <= m_elements.size())
		//ComPtr<Element> elem = cold::com::createInstance<Element>(m_elements[index]);

		return m_elements[index];
	}

	void clear(std::optional<size_t> reserve) override {
		if constexpr (IsMutable) {
			meta::ArrayValueOperations<Array>::clear(m_array, reserve);
			m_elements.clear();
			if (reserve) {
				m_elements.reserve(*reserve);
			}
		}
		else {
			RUNTIME_FAILURE("Attempt to modify non mutable value")
		}
	}

	void pushBack(RuntimeValue::Ptr value = cold::nothing) override
	{
		if constexpr (IsMutable)
		{
			//decltype(auto) element = meta::ArrayValueOperations<Array>::emplaceBack(m_array);
			//static_assert(std::is_lvalue_reference_v<decltype(element)>);
			//ComPtr<Element> elem = cold::com::createInstance<Element>(element);
			//return (*element);
			m_elements.emplace_back(runtimeValue(value));
			//return m_elements.back();
		}
	}

	RuntimeValue::Ptr back() override
	{
		return m_elements.back();
	}

private:

	T& m_array;
	std::vector<std::remove_const_t<Element>> m_elements;
};

/**
*/
template<typename T>
class RuntimeObject final : public RuntimeObjectValue
{
	COMCLASS_(RuntimeObjectValue)

	using Type = std::remove_const_t<T>;

	template<typename RtValue>
	struct FieldEntry
	{
		RtValue value;
		std::string_view name;
		bool required;

		FieldEntry(RtValue value_, std::string_view name_, bool required_)
			: value(std::move(value_))
			, name(name_)
			, required(required_)
		{}

		Field toField() {
			return {value, name};
		}

		ConstField toField() const {
			return {value, name};
		}
	};

	template<typename U>
	FieldEntry(U, std::string_view, bool) -> FieldEntry<U>;

	template<typename U, typename ... A>
	static auto makeFieldEntry(meta::FieldInfo<U, A...> fieldInfo){
		return FieldEntry{ runtimeValue(fieldInfo.value()), fieldInfo.name(), false};
	}

	template<typename ... Field>
	static auto makeRuntimeFields(std::tuple<Field...>&& fields) {
		const auto makeRuntimeFields__ = [&]<size_t ... I>(std::index_sequence<I...>) {
			return std::tuple{ makeFieldEntry(std::get<I>(fields)) ... };
		};

		return makeRuntimeFields__(Tuple::indexes(fields));
	}

	using FieldsTuple = decltype(makeRuntimeFields(meta::classFields<T>()));

public:

	RuntimeObject(T& instance_)
		: m_instance(instance_)
		, m_fields(makeRuntimeFields(meta::classFields(this->m_instance)))
	{
	}

	bool isMutable() const override 
	{
		return !std::is_const_v<T>;
	}

	bool canAdd() const override 
	{
		return false;
	}

	std::string_view typeName() const override 
	{
		return "";
	}

	size_t size() const override 
	{
		return std::tuple_size_v<FieldsTuple>;
	}

	std::string_view key(size_t index) const override 
	{
		return {};
	}

	const RuntimeValue::Ptr value(std::string_view) const override 
	{
		return cold::com::createInstance<RuntimeObject<T>>(*this);
	}

	RuntimeValue::Ptr value(std::string_view) override 
	{
		return cold::com::createInstance<RuntimeObject<T>>(*this);
	}

	RuntimeValue::Ptr add(std::string_view, RuntimeValue::Ptr value = cold::nothing) override 
	{
		RUNTIME_FAILURE("Native object values can not add more fields")
		return cold::com::createInstance<RuntimeObject<T>>(*this);
	}

	Field field(size_t index) override 
	{
		if constexpr (std::tuple_size_v<FieldsTuple> == 0) 
		{
			RUNTIME_FAILURE()
		}
		else 
		{
			std::optional<Field> field;

			Tuple::invokeAt(m_fields, index, 
				[&field](auto& fieldEntry) 
				{
					field.emplace(fieldEntry.toField());
				});

			DEBUG_CHECK(field)
			return *field;
		}
	}

	ConstField field(size_t index) const override 
	{
		std::optional<ConstField> field;
		Tuple::invokeAt(m_fields, index, [&field](const auto& fieldEntry) {
			field.emplace(fieldEntry.toField());
		});

		DEBUG_CHECK(field)
		return *field;

	}

private:

	T& m_instance;
	FieldsTuple m_fields;
};



} // namespace cold_internal

template<typename T>
ComPtr<cold_internal::RuntimePrimitive<T>> runtimeValue(T& instance)
{
	return cold::com::createInstance<cold_internal::RuntimePrimitive<T>>(instance);
}

template<typename T>
ComPtr<cold_internal::RuntimePrimitive<const T>> runtimeValue(const T& instance)
{
	return cold::com::createInstance<cold_internal::RuntimePrimitive<const T>>(instance);
}


template<meta::OptionalRepresentable T>
ComPtr<cold_internal::RuntimeOptional<T>> runtimeValue(T& instance)
{
	return cold::com::createInstance<cold_internal::RuntimeOptional<T>>(instance);
}

template<meta::OptionalRepresentable T>
ComPtr<cold_internal::RuntimeOptional<const T>> runtimeValue(const T& instance)
{
	return cold::com::createInstance<cold_internal::RuntimeOptional<const T>>(instance);
}

template<meta::ArrayRepresentable T>
ComPtr<cold_internal::RuntimeArray<T>> runtimeValue(T& instance)
{
	return cold::com::createInstance<cold_internal::RuntimeArray<T>>(instance);
}

template<meta::ArrayRepresentable T>
ComPtr<cold_internal::RuntimeArray<const T>> runtimeValue(const T& instance)
{
	return cold::com::createInstance<cold_internal::RuntimeArray<const T>>(instance);
}

template<meta::ObjectRepresentable T>
ComPtr<cold_internal::RuntimeObject<T>> runtimeValue(T& instance)
{
	return cold::com::createInstance<cold_internal::RuntimeObject<T>>(instance);
}

template<meta::ObjectRepresentable T>
ComPtr<cold_internal::RuntimeObject<const T>> runtimeValue(const T& instance)
{
	return cold::com::createInstance<cold_internal::RuntimeObject<const T>>(instance);
}



} // namespace cold