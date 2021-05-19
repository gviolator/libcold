#pragma once
#include <cold/com/ianything.h>
#include <cold/com/interface.h>
#include <cold/meta/classinfo.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/runtime/runtimeexport.h>
#include <cold/utils/result.h>
#include <cold/com/comptr.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>


namespace cold {

/**

*/
struct ABSTRACT_TYPE RuntimeValue : IRefCounted
{
	using Ptr = ComPtr<RuntimeValue>;

	RUNTIME_EXPORT static Result<> assign(RuntimeValue::Ptr dst, const RuntimeValue::Ptr src);

	virtual ~RuntimeValue() = default;

	virtual bool isMutable() const = 0;
};

/**
*/
struct RuntimePrimitiveValue : RuntimeValue
{
	DECLARE_CLASS_BASE(RuntimeValue)

protected:
	RuntimePrimitiveValue() = default;
};

/**

*/
template<typename T>
struct ABSTRACT_TYPE TypedRuntimePrimitiveValue : RuntimePrimitiveValue
{
	DECLARE_CLASS_BASE(RuntimePrimitiveValue)

	virtual Result<> set(T value) = 0;

	virtual T get() const = 0;

	T operator * () const {
		return this->get();
	}

protected:
	TypedRuntimePrimitiveValue() = default;
};



using RuntimeBooleanValue = TypedRuntimePrimitiveValue<bool>;
using RuntimeUInt8Value = TypedRuntimePrimitiveValue<unsigned char>;
using RuntimeUInt16Value = TypedRuntimePrimitiveValue<unsigned short>;
using RuntimeUInt32Value = TypedRuntimePrimitiveValue<unsigned int>;
using RuntimeUInt64Value = TypedRuntimePrimitiveValue<uint64_t>;
using RuntimeInt8Value = TypedRuntimePrimitiveValue<char>;
using RuntimeInt16Value = TypedRuntimePrimitiveValue<short int>;
using RuntimeInt32Value = TypedRuntimePrimitiveValue<int>;
using RuntimeInt64Value = TypedRuntimePrimitiveValue<int64_t>;
using RuntimeSingleValue = TypedRuntimePrimitiveValue<float>;
using RuntimeDoubleValue = TypedRuntimePrimitiveValue<double>;
using RuntimeStringValue = TypedRuntimePrimitiveValue<std::string>;
using RuntimeWStringValue = TypedRuntimePrimitiveValue<std::wstring>;

/**

*/
struct ABSTRACT_TYPE RuntimeOptionalValue : RuntimeValue
{
	DECLARE_CLASS_BASE(RuntimeValue)

	virtual bool hasValue() const = 0;

	virtual const RuntimeValue::Ptr value() const = 0;

	virtual RuntimeValue::Ptr value() = 0;

	virtual void reset() = 0;

	virtual RuntimeValue::Ptr emplace(RuntimeValue::Ptr value = cold::nothing) = 0;

	explicit inline operator bool () const {
		return this->hasValue();
	}
};

/**
*/
struct ABSTRACT_TYPE RuntimeReadonlyCollection : RuntimeValue
{
	DECLARE_CLASS_BASE(RuntimeValue)

	virtual size_t size() const = 0;

	virtual const RuntimeValue::Ptr element(size_t index) const = 0;

	virtual RuntimeValue::Ptr element(size_t index) = 0;

	inline const RuntimeValue::Ptr operator[](size_t index) const
	{
		return this->element(index);
	}

	inline RuntimeValue::Ptr operator[](size_t index) {
		return this->element(index);
	}

protected:
	using RuntimeValue::RuntimeValue;
};


/**
*/
struct ABSTRACT_TYPE RuntimeArrayValue : RuntimeReadonlyCollection
{
	DECLARE_CLASS_BASE(RuntimeReadonlyCollection)

	virtual void clear(std::optional<size_t> reserve = std::nullopt) = 0;

	virtual void pushBack(RuntimeValue::Ptr value = cold::nothing) = 0;

	virtual RuntimeValue::Ptr back() = 0;
};

/**
*/
struct ABSTRACT_TYPE RuntimeTupleValue : RuntimeReadonlyCollection
{
	DECLARE_CLASS_BASE(RuntimeReadonlyCollection)
};


struct ABSTRACT_TYPE RuntimeNamedValueCollection : RuntimeValue
{
	DECLARE_CLASS_BASE(RuntimeValue)

	virtual bool canAdd() const = 0;

	virtual size_t size() const = 0;

	virtual std::string_view key(size_t index) const = 0;

	virtual const RuntimeValue::Ptr value(std::string_view) const = 0;

	virtual RuntimeValue::Ptr value(std::string_view) = 0;

	virtual RuntimeValue::Ptr add(std::string_view, RuntimeValue::Ptr value = cold::nothing) = 0;

	/*inline const RuntimeValue::Ptr operator[](std::string_view key) const {
		return this->value(key);
	}*/

	inline RuntimeValue::Ptr operator[](std::string_view key) {
		return this->value(key);
	}

protected:
	using RuntimeValue::RuntimeValue;
};


/**
	Generalized object runtime representation.
*/
struct ABSTRACT_TYPE RuntimeObjectValue : RuntimeNamedValueCollection
{
	DECLARE_CLASS_BASE(RuntimeNamedValueCollection)

	/**
	*/
	class Field
	{
	public:
		Field() = default;

		Field(RuntimeValue::Ptr value_, std::string_view name_): m_value(std::move(value_)), m_name(name_)
		{}
		
		explicit operator bool () const {
			return static_cast<bool>(m_value);
		}

		RuntimeValue::Ptr value() const {
			DEBUG_CHECK(m_value)
			return m_value;
		}

		std::string_view name() const 
		{
			return m_name;
		}

	private:
		RuntimeValue::Ptr m_value = cold::nothing;
		std::string_view m_name;
	};

	/**
	*/
	class ConstField
	{
	public:
		ConstField() = default;

		ConstField(const RuntimeValue::Ptr& value_, std::string_view name_) : m_value(value_), m_name(name_)
		{}

		ConstField(const Field& field_) : m_value(field_ ? field_.value() : cold::nothing), m_name(field_.name())
		{}
		
		explicit operator bool () const {
			return static_cast<bool>(m_value);
		}

		const RuntimeValue::Ptr value() const {
			DEBUG_CHECK(m_value)
			return m_value;
		}

		std::string_view name() const {
			return m_name;
		}

	private:
		const RuntimeValue::Ptr m_value = cold::nothing;
		std::string_view m_name;
	};

	virtual std::string_view typeName() const = 0;

	virtual Field field(size_t index) = 0;

	virtual ConstField field(size_t index) const = 0;

	std::optional<Field> operator[](std::string_view) {
		return std::nullopt;
	}

	std::optional<ConstField> operator[](std::string_view) const {
		return std::nullopt;
	}
};

// struct ABSTRACT_TYPE RuntimeDictionaryValue : RuntimeNamedValueCollection
// {
// 	DECLARE_BASE(RuntimeValue)

// protected:
// 	RuntimeDictionaryValue(): RuntimeNamedValueCollection(RuntimeValueCategory::Dictionary)
// 	{}
// };

}
