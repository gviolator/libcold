#pragma once
#include <cold/com/ianything.h>
#include <cold/com/interface.h>
#include <cold/meta/classinfo.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/runtime/runtimeexport.h>
#include <cold/utils/result.h>
#include <cold/utils/stringconv.h>
#include <cold/com/comptr.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace cold {

/**

*/
struct ABSTRACT_TYPE RuntimeValue : virtual IRefCounted
{
	using Ptr = ComPtr<RuntimeValue>;

	RUNTIME_EXPORT static Result<> assign(RuntimeValue::Ptr dst, RuntimeValue::Ptr src);

	virtual ~RuntimeValue() = default;

	virtual bool isMutable() const = 0;
};

/**
*/
struct ABSTRACT_TYPE RuntimePrimitiveValue : virtual RuntimeValue
{
	DECLARE_CLASS_BASE(RuntimeValue)
};


struct ABSTRACT_TYPE RuntimeStringValue : virtual RuntimePrimitiveValue
{
	DECLARE_CLASS_BASE(RuntimePrimitiveValue)

	virtual void setUtf8(std::string_view) = 0;

	virtual void set(std::wstring) = 0;

	virtual std::wstring get() const = 0;

	virtual std::string getUtf8() const = 0;
};



struct ABSTRACT_TYPE RuntimeIntegerValue : virtual RuntimePrimitiveValue
{
	DECLARE_CLASS_BASE(RuntimePrimitiveValue)

	virtual bool isSigned() const = 0;

	virtual size_t bits() const = 0;

	virtual void setInt64(int64_t) = 0;

	virtual void setUint64(uint64_t) = 0;

	virtual int64_t getInt64() const = 0;

	virtual uint64_t getUint64() const = 0;


	template<typename T>
	void set(T value) requires(std::is_integral_v<T>)
	{
		//this->set(&value, sizeof(T));
	}

	template<typename T>
	T get() requires(std::is_integral_v<T>)
	{
		//T value = 0;
		//this->get(&value, sizeof(T));

		return 0;
	}
};


struct ABSTRACT_TYPE RuntimeFloatValue : virtual RuntimePrimitiveValue
{
	DECLARE_CLASS_BASE(RuntimePrimitiveValue)

	virtual size_t bits() const = 0;

	virtual void setDouble(double) = 0;

	virtual void setSingle(float) = 0;

	virtual double getDouble() const = 0;
	
	virtual float getSingle() const = 0;

	template<typename T>
	T get() const requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
	{
		return static_cast<T>(this->getDouble());
	}
};


struct ABSTRACT_TYPE RuntimeBooleanValue : virtual RuntimePrimitiveValue
{
	DECLARE_CLASS_BASE(RuntimePrimitiveValue)

	virtual void set(bool) = 0;

	virtual bool get() const = 0;
};


/**

*/
struct ABSTRACT_TYPE RuntimeOptionalValue : virtual RuntimeValue
{
	DECLARE_CLASS_BASE(RuntimeValue)

	virtual bool hasValue() const = 0;

	virtual RuntimeValue::Ptr value() const = 0;

	virtual Result<> setValue(RuntimeValue::Ptr value = cold::nothing) = 0;

	explicit inline operator bool () const
	{
		return this->hasValue();
	}
};

/**
*/
struct ABSTRACT_TYPE RuntimeReadonlyCollection : virtual RuntimeValue
{
	DECLARE_CLASS_BASE(RuntimeValue)

	virtual size_t size() const = 0;

	virtual RuntimeValue::Ptr element(size_t index) const = 0;

	inline RuntimeValue::Ptr operator[](size_t index) const
	{
		return this->element(index);
	}
};


/**
*/
struct ABSTRACT_TYPE RuntimeArray : virtual RuntimeReadonlyCollection
{
	DECLARE_CLASS_BASE(RuntimeReadonlyCollection)

	virtual void clear() = 0;

	virtual void reserve(size_t) = 0;

	virtual Result<> push(RuntimeValue::Ptr) = 0;
};


/**
*/
struct ABSTRACT_TYPE RuntimeTuple : virtual RuntimeReadonlyCollection
{
	DECLARE_CLASS_BASE(RuntimeReadonlyCollection)
};


/**
*/
struct ABSTRACT_TYPE RuntimeReadonlyDictionary : virtual RuntimeValue
{
	DECLARE_CLASS_BASE(RuntimeValue)


	virtual size_t size() const = 0;

	virtual std::string_view key(size_t index) const = 0;

	virtual RuntimeValue::Ptr value(std::string_view) const = 0;

	virtual bool hasKey(std::string_view) const = 0;


	inline RuntimeValue::Ptr operator[](std::string_view key) const
	{
		return this->value(key);
	}

	inline std::pair<std::string_view, RuntimeValue::Ptr> operator[](size_t index) const
	{
		auto key_ = this->key(index);
		return {key_, this->value(key_)};
	}
};


/**
* 
*/
struct ABSTRACT_TYPE RuntimeDictionary : virtual RuntimeReadonlyDictionary
{
	DECLARE_CLASS_BASE(RuntimeReadonlyDictionary)


	virtual void clear() = 0;

	virtual Result<> set(std::string_view, RuntimeValue::Ptr value) = 0;

	virtual RuntimeValue::Ptr erase(std::string_view) = 0;
};


/**
	Generalized object runtime representation.
*/
struct ABSTRACT_TYPE RuntimeObject : virtual RuntimeReadonlyDictionary
{
	DECLARE_CLASS_BASE(RuntimeReadonlyDictionary)

	struct FieldInfo
	{

	};

	virtual Result<> set(std::string_view, RuntimeValue::Ptr value) = 0;

	virtual std::optional<FieldInfo> fieldInfo(std::string_view) const  = 0;
};



}
