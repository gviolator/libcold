#pragma once
#include <cold/serialization/runtimevalue.h>
#include <cold/serialization/valuerepresentation.h>
#include <cold/serialization/valuerepresentation/arrayrepresentation.h>
#include <cold/serialization/valuerepresentation/optionalrepresentation.h>
#include <cold/serialization/valuerepresentation/dictionaryrepresentation.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/strings.h>
#include <cold/utils/result.h>
#include <cold/utils/typeutility.h>
#include <cold/com/comclass.h>

namespace cold {


//-----------------------------------------------------------------------------
template<typename T>
ComPtr<RuntimeIntegerValue> runtimeValue(T&) requires (std::is_integral_v<T>);

template<typename T>
ComPtr<RuntimeIntegerValue> runtimeValue(const T&) requires (std::is_integral_v<T>);

template<typename T>
ComPtr<RuntimeFloatValue> runtimeValue(T&) requires (std::is_floating_point_v<T>);

template<typename T>
ComPtr<RuntimeFloatValue> runtimeValue(const T&) requires (std::is_floating_point_v<T>);

template<meta::OptionalRepresentable T>
ComPtr<RuntimeOptionalValue> runtimeValue(T&);

template<meta::OptionalRepresentable T>
ComPtr<RuntimeOptionalValue> runtimeValue(const T&);

template<meta::ArrayRepresentable T>
ComPtr<RuntimeArray> runtimeValue(T&);

template<meta::ArrayRepresentable T>
ComPtr<RuntimeArray> runtimeValue(const T&);

template<meta::TupleRepresentable T>
ComPtr<RuntimeTuple> runtimeValue(T&);

template<meta::TupleRepresentable  T>
ComPtr<RuntimeTuple> runtimeValue(const T&);

template<meta::DictionaryRepresentable T>
ComPtr<RuntimeDictionary> runtimeValue(T&);

template<meta::DictionaryRepresentable T>
ComPtr<RuntimeDictionary> runtimeValue(const T&);

template<meta::ObjectRepresentable T>
ComPtr<RuntimeObject> runtimeValue(T&);

template<meta::ObjectRepresentable T>
ComPtr<RuntimeObject> runtimeValue(const T&);


//-----------------------------------------------------------------------------
/**
* 
*/
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


/**
* 
*/
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

	void setDouble([[maybe_unused]] double value) override
	{
		if constexpr (!IsMutable)
		{
			RUNTIME_FAILURE("Attempt to modify non mutable runtime value")
		}
		else
		{
			m_value = static_cast<T>(value);
		}
	}

	void setSingle([[maybe_unused]] float value) override
	{
		if constexpr (!IsMutable)
		{
			RUNTIME_FAILURE("Attempt to modify non mutable runtime value")
		}
		else
		{
			m_value = static_cast<T>(value);
		}
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


template<meta::OptionalRepresentable T>
class NativeOptionalValue final : public RuntimeOptionalValue
{
	COMCLASS_(RuntimeOptionalValue)
public:

	static constexpr bool IsMutable = !std::is_const_v<T>;

	NativeOptionalValue(T& optionalValue) : m_optionalValue(optionalValue)
	{}

	bool isMutable() const override
	{
		return IsMutable;
	}

	bool hasValue() const override
	{
		return meta::OptionalValueOperations<T>::hasValue(m_optionalValue);
	}

	RuntimeValue::Ptr value() const override
	{
		if (!this->hasValue())
		{
			return nothing;
		}

		decltype(auto) value = meta::OptionalValueOperations<T>::value(m_optionalValue);
		return runtimeValue(value);
	}

	Result<> setValue(RuntimeValue::Ptr value_) override
	{
		if (!value_)
		{
			meta::OptionalValueOperations<T>::reset(m_optionalValue);
			return success;
		}

		using OptionalOp = meta::OptionalValueOperations<T>;
		
		decltype(auto) myValue = OptionalOp::hasValue(m_optionalValue) ? OptionalOp::value(m_optionalValue) : OptionalOp::emplace(m_optionalValue);

		return RuntimeValue::assign(cold::runtimeValue(myValue), std::move(value_));
	}

private:

	T& m_optionalValue;
};


/**
*/
template<typename T>
class NativeArray final : public RuntimeArray
{
	COMCLASS_(RuntimeArray)

	using ContainerType = std::remove_const_t<T>;

public:

	static constexpr bool IsMutable = !std::is_const_v<T>;

	NativeArray(T& array_): m_array(array_)
	{
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
		DEBUG_CHECK(index < this->size())

		return runtimeValue(m_array[index]);
	}

	void clear() override
	{
		if constexpr (!IsMutable)
		{
			RUNTIME_FAILURE("Attempt to modify non mutable value")
		}
		else
		{
			meta::ArrayValueOperations<ContainerType>::clear(m_array, std::nullopt);
		}
	}

	void reserve(size_t) override
	{

	}

	Result<> push(RuntimeValue::Ptr value) override
	{
		if constexpr (!IsMutable)
		{
			RUNTIME_FAILURE("Attempt to modify non mutable array")
		}
		else
		{
			decltype(auto) newElement = meta::ArrayValueOperations<ContainerType>::emplaceBack(m_array);
			return RuntimeValue::assign(runtimeValue(newElement), std::move(value));
		}

		return success;
	}

private:

	T& m_array;
};


template<meta::TupleRepresentable T>
class NativeTuple final : public RuntimeTuple
{
	COMCLASS_(RuntimeTuple)

public:
	static constexpr bool IsMutable = !std::is_const_v<T>;
	static constexpr size_t Size = meta::TupleValueOperations<T>::TupleSize;

	NativeTuple(T& value): m_value(value)
	{}

	bool isMutable() const override
	{
		return IsMutable;
	}

	size_t size() const override
	{
		return Size;
	}

	RuntimeValue::Ptr element(size_t index) const override
	{
		DEBUG_CHECK(index < Size)

		using F = RuntimeValue::Ptr(*) (T&);

		const auto elementFactories = [&]<size_t ... I>(std::index_sequence<I...>)
		{
			return std::array<F, Size> { &createElementValue<I> ... };
		};

		const F f = elementFactories(std::make_index_sequence<Size>{})[index];
		return f(m_value);
	}

private:

	template<size_t I>
	static RuntimeValue::Ptr createElementValue(T& container)
	{
		decltype(auto) el = meta::TupleValueOperations<T>::element<I>(container);
		return runtimeValue(el);
	}

	T& m_value;
};


template<meta::DictionaryRepresentable T>
class NativeDictionary final : public RuntimeDictionary
{
	COMCLASS_(RuntimeDictionary)

public:
	static constexpr bool IsMutable = !std::is_const_v<T>;
	using Key = typename meta::DictionaryValueOperations<T>::Key;
	using Value= typename meta::DictionaryValueOperations<T>::Value;

	NativeDictionary(T& container): m_container(container)
	{}

	bool isMutable() const override
	{
		return IsMutable;
	}

	size_t size() const override
	{
		return meta::DictionaryValueOperations<T>::size(m_container);
	}

	std::string_view key(size_t index) const override
	{
		decltype(auto) k = meta::DictionaryValueOperations<T>::key(m_container, index);

		return k;
	}

	RuntimeValue::Ptr value(std::string_view key) const override
	{
		std::conditional_t<IsMutable, Value, const Value>* value = nullptr;

		if (!meta::DictionaryValueOperations<T>::find(m_container, Key{key}, &value))
		{
			return nothing;
		}

		return runtimeValue(*value);
	}

	bool hasKey(std::string_view key) const override
	{
		return meta::DictionaryValueOperations<T>::find(m_container, Key{key});
	}

	void clear() override
	{
		if constexpr (!IsMutable)
		{
			RUNTIME_FAILURE("Attempt to modify non mutable dictionary value")
		}
		else
		{
			meta::DictionaryValueOperations<T>::clear(m_container);
		}
	}

	Result<> set(std::string_view key, RuntimeValue::Ptr value) override
	{
		if constexpr (!IsMutable)
		{
			RUNTIME_FAILURE("Attempt to modify non mutable dictionary value")
		}
		else
		{
			Value* myValue = nullptr;
			if (!meta::DictionaryValueOperations<T>::find(m_container, Key{key}, &myValue));
			{
				myValue = &meta::DictionaryValueOperations<T>::emplace(m_container, Key{key});
			}

			DEBUG_CHECK(myValue )

			return RuntimeValue::assign(runtimeValue(*myValue), value);
		}

		return success;
	}

	RuntimeValue::Ptr erase(std::string_view) override
	{
		if constexpr (!IsMutable)
		{
			RUNTIME_FAILURE("Attempt to modify non mutable dictionary value")
		}
		else
		{
			
		}

		return nothing;
	}

private:

	T& m_container;
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

		return Field(field.name(), reinterpret_cast<void*>(const_cast<std::remove_const_t<FieldValueType>*>(&fieldValue)), factory);
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

//-----------------------------------------------------------------------------
// Integer
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

// Floating point
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

// Optional
template<meta::OptionalRepresentable T>
ComPtr<RuntimeOptionalValue> runtimeValue(T& optionalValue)
{
	return com::createInstance<NativeOptionalValue<T>, RuntimeOptionalValue>(optionalValue);
}

template<meta::OptionalRepresentable T>
ComPtr<RuntimeOptionalValue> runtimeValue(const T& value)
{
	return com::createInstance<NativeOptionalValue<const T>, RuntimeOptionalValue>(value);
}

// Array
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

// Tuple
template<meta::TupleRepresentable T>
ComPtr<RuntimeTuple> runtimeValue(T& container)
{
	return com::createInstance<NativeTuple<T>, RuntimeTuple>(container);
}

template<meta::TupleRepresentable  T>
ComPtr<RuntimeTuple> runtimeValue(const T& container)
{
	return com::createInstance<NativeTuple<const T>, RuntimeTuple>(container);
}

// Dictionary
template<meta::DictionaryRepresentable T>
ComPtr<RuntimeDictionary> runtimeValue(T& container)
{
	return com::createInstance<NativeDictionary<T>, RuntimeDictionary>(container);
}

template<meta::DictionaryRepresentable T>
ComPtr<RuntimeDictionary> runtimeValue(const T& container)
{
	return com::createInstance<NativeDictionary<const T>, RuntimeDictionary>(container);
}

// Object
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
