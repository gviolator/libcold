#pragma once
#include <cold/serialization/runtimevalue.h>
#include <cold/serialization/valuerepresentation.h>
#include <cold/serialization/valuerepresentation/arrayrepresentation.h>
#include <cold/serialization/valuerepresentation/optionalrepresentation.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/strings.h>
#include <cold/utils/result.h>
#include <cold/utils/typeutility.h>
#include <cold/com/comclass.h>

namespace cold {



template<typename T>
ComPtr<RuntimeIntegerValue> runtimeValue(T&) requires (std::is_integral_v<T>);

template<typename T>
ComPtr<RuntimeIntegerValue> runtimeValue(const T&) requires (std::is_integral_v<T>);

template<typename T>
ComPtr<RuntimeFloatValue> runtimeValue(T&) requires (std::is_floating_point_v<T>);

template<typename T>
ComPtr<RuntimeFloatValue> runtimeValue(const T&) requires (std::is_floating_point_v<T>);


template<meta::ArrayRepresentable T>
ComPtr<RuntimeArray> runtimeValue(T&);

template<meta::ArrayRepresentable T>
ComPtr<RuntimeArray> runtimeValue(const T&);


template<meta::ObjectRepresentable T>
ComPtr<RuntimeObject> runtimeValue(T&);

template<meta::ObjectRepresentable T>
ComPtr<RuntimeObject> runtimeValue(const T&);

// namespace cold_internal {

//template<typename T, typename Base>
//requires(std::is_base_of_v<RuntimeValue, Base>)
//class NativeValueBase : public virtual Base
//{
//	DECLARE_CLASS_BASE(RuntimeValue)
//
//public:
//
//	static constexpr IsConst = std::is_const_v<T>;
//	using ValueType = std::remove_const_t<T>;
//
//	bool isMutable() const override
//	{
//		return !IsConst;
//	}
//};


template<typename T>
requires (std::is_integral_v<T>)
class NativeIntegerValue final : public RuntimeIntegerValue
{
	COMCLASS_(RuntimeIntegerValue)

public:
	static constexpr bool IsMutable = !std::is_const_v<T>;
	using ValueType = std::remove_const_t<T>;

	NativeIntegerValue(T& value): m_value(value)
	{}

	bool isMutable() const override
	{
		return IsMutable;
	}

	bool isSigned() const override
	{
		return std::is_signed_v<T>;
	}

	size_t bits() const override
	{
		return sizeof(T);
	}

	void setInt64(int64_t value) override
	{
		m_value = static_cast<T>(value);
	}

	void setUint64(uint64_t value) override
	{
		m_value = static_cast<T>(value);
	}

	int64_t getInt64() const override
	{
		return static_cast<int64_t>(m_value);
	}

	uint64_t getUint64() const override
	{
		return static_cast<uint64_t>(m_value);
	}

private:
	T& m_value;
};



template<typename T>
requires (std::is_floating_point_v<T>)
class NativeFloatValue final : public RuntimeFloatValue
{
	COMCLASS_(RuntimeFloatValue)

public:
	static constexpr bool IsMutable = !std::is_const_v<T>;
	using ValueType = std::remove_const_t<T>;

	NativeFloatValue(T& value): m_value(value)
	{}

	bool isMutable() const override
	{
		return IsMutable;
	}

	size_t bits() const override
	{
		return sizeof(T);
	}

	void setDouble(double value) override
	{
		m_value = static_cast<T>(value);
	}

	void setSingle(float value) override
	{
		m_value = static_cast<T>(value);
	}

	double getDouble() const override
	{
		return static_cast<double>(m_value);
	}
	
	float getSingle() const override
	{
		return static_cast<float>(m_value);
	}

private:
	T& m_value;
};


/**
*/
template<meta::ArrayRepresentable T>
class NativeArray final : public RuntimeArray
{
	COMCLASS_(RuntimeArray)

	using ContainerType = std::remove_const_t<T>;

	static constexpr bool IsMutable = !std::is_const_v<T>;
public:

	NativeArray(T& array_): m_array(array_)
	{
		// using namespace cold::meta;

		//m_elements.reserve(ArrayValueOperations<Array>::size(m_array));
		//for (auto& element : m_array) {
		//	[[maybe_unused]]Element& el = m_elements.emplace_back(runtimeValue(element));
		//	DEBUG_CHECK(el->isMutable() == IsMutable)
		//}
	}

	bool isMutable() const override
	{
		return IsMutable;
	}

	size_t size() const override
	{
		return meta::ArrayValueOperations<ContainerType>::size(m_array);
	}

	RuntimeValue::Ptr element(size_t index) const override
	{
		// DEBUG_CHECK(m_elements.size() == m_array.size() && index <= m_elements.size())

		return runtimeValue(m_array[index]);

		//ComPtr<const RuntimeValue> elem = cold::com::createInstance<const Element>(m_elements[index]);

		// return m_elements[index];
	}

	//RuntimeValue::Ptr element(size_t index) override {
	//	DEBUG_CHECK(m_elements.size() == m_array.size() && index <= m_elements.size())
	//	//ComPtr<Element> elem = cold::com::createInstance<Element>(m_elements[index]);

	//	return m_elements[index];
	//}

	void clear() override
	{
		if constexpr (IsMutable)
		{
			// meta::ArrayValueOperations<ContainerType>::clear(m_array);
		/*	m_elements.clear();
			if (reserve) {
				m_elements.reserve(*reserve);
			}*/
		}
		else {
			RUNTIME_FAILURE("Attempt to modify non mutable value")
		}
	}

	void reserve(size_t) override
	{

	}

	Result<> push(RuntimeValue::Ptr value) override
	{
		if constexpr (IsMutable)
		{
			decltype(auto) newElement = meta::ArrayValueOperations<ContainerType>::emplaceBack(m_array);
			auto ignore = RuntimeValue::assign(runtimeValue(newElement), std::move(value));
		}

		return success;
	}

private:

	T& m_array;
};


template<meta::ObjectRepresentable T>
class NativeObject : public RuntimeObject
{
	COMCLASS_(RuntimeObject)

public:

	static constexpr bool IsMutable = !std::is_const_v<T>;

	NativeObject(T& value): m_value(value), m_fields(makeFields(meta::classFields(this->m_value)))
	{}

	bool isMutable() const override
	{
		return IsMutable;
	}

	size_t size() const override
	{
		return NativeObject<T>::FieldsCount;
	}

	std::string_view key(size_t index) const override
	{
		return m_fields[index].name();
	}

	RuntimeValue::Ptr value(std::string_view key_) const override
	{
		auto iter = std::find_if(m_fields.begin(), m_fields.end(), [key_](const Field& field) { return strings::icaseEqual(field.name(), key_); });

		return iter != m_fields.end() ? iter->value() : nothing;
	}

	bool hasKey(std::string_view key_) const override
	{
		auto iter = std::find_if(m_fields.begin(), m_fields.end(), [key_](const Field& field) { return strings::icaseEqual(field.name(), key_); });

		return iter != m_fields.end();
	}

	Result<> set(std::string_view, RuntimeValue::Ptr value) override
	{
		return success;
	}

	std::optional<FieldInfo> fieldInfo(std::string_view) const override
	{
		return std::nullopt;
	}

private:

	using FieldValueFactory = RuntimeValue::Ptr (*) (void*) noexcept;

	class Field
	{
	public:
		Field(std::string_view name_, void* value_, FieldValueFactory factory): m_name(name_), m_fieldValue(value_), m_factory(factory)
		{}

		std::string_view name() const
		{
			return m_name;
		}

		const RuntimeValue::Ptr& value() const
		{
			if (!m_rtValue)
			{
				m_rtValue = m_factory(m_fieldValue);
			}

			return m_rtValue;
		}

	private:
		const std::string_view m_name;
		void* const m_fieldValue;
		const FieldValueFactory m_factory;
		mutable RuntimeValue::Ptr m_rtValue;
		

	};

	template<typename FieldValueType, typename ... Attribs>
	static Field makeField(meta::FieldInfo<FieldValueType, Attribs...> field)
	{
		const FieldValueFactory factory = [](void* ptr) noexcept -> RuntimeValue::Ptr
		{
			FieldValueType* const value = reinterpret_cast<FieldValueType*>(ptr);
			return runtimeValue(*value);
		};

		FieldValueType& fieldValue = field.value();

		return Field(field.name(), reinterpret_cast<void*>(&fieldValue), factory);
	}


	template<typename ... FieldInfo>
	static auto makeFields(std::tuple<FieldInfo...>&& fields)
	{
		const auto makeFieldsHelper = [&]<size_t ...I>(std::index_sequence<I...>)
		{
			return std::array { makeField(std::get<I>(fields)) ...};
		};

		return makeFieldsHelper(Tuple::indexes(fields));
	}


	using Fields = decltype(NativeObject<T>::makeFields(meta::classFields<T>()));

	static constexpr size_t FieldsCount = std::tuple_size_v<Fields>;



	T& m_value;
	Fields m_fields;
};


//template<typename T>
//class RuntimePrimitive;
//
//template<typename T>
//class RuntimeOptional;
//
//template<typename>
//class RuntimeArray;
//
//template<typename T>
//class RuntimeObject;


// } // namespace cold_internal



//// objects
//template<meta::ObjectRepresentable T>
//ComPtr<cold_internal::RuntimeObject<T>> runtimeValue(T&);
//
//template<meta::ObjectRepresentable T>
//ComPtr<cold_internal::RuntimeObject<const T>> runtimeValue(const T&);
//
//// arrays
//template<meta::ArrayRepresentable T>
//ComPtr<cold_internal::RuntimeArray<T>> runtimeValue(T&);
//
//template<meta::ArrayRepresentable T>
//ComPtr<cold_internal::RuntimeArray<const T>> runtimeValue(const T&);
//
//// optionals
//template<meta::OptionalRepresentable T>
//ComPtr<cold_internal::RuntimeOptional<T>> runtimeValue(T&);
//
//template<meta::OptionalRepresentable T>
//ComPtr<cold_internal::RuntimeOptional<const T>> runtimeValue(const T&);
//
//// primitives
//template<typename T>
//ComPtr<cold_internal::RuntimePrimitive<T>> runtimeValue(T&);
//
//template<typename T>
//ComPtr<cold_internal::RuntimePrimitive<const T>> runtimeValue(const T&);





namespace cold_internal {

template<typename T>
using RuntimeValueTypeOf = decltype(cold::runtimeValue(std::declval<T>()));

#if 0

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

#endif

} // namespace cold_internal


#if 0

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


#endif


template<typename T>
ComPtr<RuntimeIntegerValue> runtimeValue(T& value) requires (std::is_integral_v<T>)
{
	return com::createInstance<NativeIntegerValue<T>, RuntimeIntegerValue>(value);
}

template<typename T>
ComPtr<RuntimeIntegerValue> runtimeValue(const T& value) requires (std::is_integral_v<T>)
{
	return com::createInstance<NativeIntegerValue<const T>, RuntimeIntegerValue>(value);
}


template<typename T>
ComPtr<RuntimeFloatValue> runtimeValue(T& value) requires (std::is_floating_point_v<T>)
{
	return com::createInstance<NativeFloatValue<T>, RuntimeFloatValue>(value);
}

template<typename T>
ComPtr<RuntimeFloatValue> runtimeValue(const T& value) requires (std::is_floating_point_v<T>)
{
	return com::createInstance<NativeFloatValue<const T>, RuntimeFloatValue>(value);
}


template<meta::ArrayRepresentable T>
ComPtr<RuntimeArray> runtimeValue(T& container)
{
	return com::createInstance<NativeArray<T>, RuntimeArray>(container);
}

template<meta::ArrayRepresentable T>
ComPtr<RuntimeArray> runtimeValue(const T& container)
{
	return com::createInstance<NativeArray<const T>, RuntimeArray>(container);
}


template<meta::ObjectRepresentable T>
ComPtr<RuntimeObject> runtimeValue(T& value)
{
	return com::createInstance<NativeObject<T>, RuntimeObject>(value);
}

template<meta::ObjectRepresentable T>
ComPtr<RuntimeObject> runtimeValue(const T& value)
{
	return com::createInstance<NativeObject<const T>, RuntimeObject>(value);
}

} // namespace cold