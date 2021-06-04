#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/diagnostics/sourceinfo.h>
#include <cold/diagnostics/exception.h>
#include <cold/meta/classinfo.h>
#include <cold/com/ianything.h>
#include <cold/io/writer.h>
#include <cold/io/reader.h>
#include <cold/serialization/serialization.h>
#include <cold/utils/result.h>
#include <cold/meta/attribute.h>
#include <cold/serialization/runtimevalue.h>
#include <cold/com/comptr.h>


namespace cold {
namespace serialization_internal {

class SerializationExceptionImpl;
class RequiredFieldMissedExceptionImpl;
class InvalidTypeExceptionImpl;
class NumericOverflowExcepionImpl;
class EndOfStreamExceptionImpl;

} // namespace serialization_internal

namespace serialization {

/**
*/
struct INTERFACE_API ISerialization : virtual IAnything
{
	/**
	*/
	virtual Result<> serialize(io::Writer& writer, const RuntimeValue::Ptr) const = 0;

	/**
	*/
	virtual Result<RuntimeValue::Ptr> deserialize(io::Reader&) const = 0;

	/**
	*/
	virtual Result<> deserializeInplace(io::Reader&, RuntimeValue::Ptr target) const = 0;
};

/**
*/
class ABSTRACT_TYPE SerializationException : public Exception
{
	DECLARE_ABSTRACT_EXCEPTION(Exception, cold::serialization_internal::SerializationExceptionImpl)
};

/**
*/
class ABSTRACT_TYPE RequiredFieldMissedException : public SerializationException
{
	DECLARE_ABSTRACT_EXCEPTION(SerializationException, cold::serialization_internal::RequiredFieldMissedExceptionImpl)

public:
	virtual std::string fieldName() const = 0;
	virtual std::string typeName() const = 0;
};

/**
*/
class ABSTRACT_TYPE InvalidTypeException : public SerializationException
{
	DECLARE_ABSTRACT_EXCEPTION(SerializationException, cold::serialization_internal::InvalidTypeExceptionImpl)
public:
	virtual std::string expectedTypeName() const = 0;
	virtual std::string actualTypeName() const = 0;
};

/**
*/
class NumericOverflowExcepion : public SerializationException
{
	DECLARE_ABSTRACT_EXCEPTION(SerializationException, cold::serialization_internal::NumericOverflowExcepionImpl)
public:
};

/**
*/
class EndOfStreamException : public SerializationException
{
	DECLARE_ABSTRACT_EXCEPTION(SerializationException, cold::serialization_internal::EndOfStreamExceptionImpl)
};

/**
*/
struct RequiredFieldAttribute : meta::Attribute
{};

/**
*/
struct IgnoreEmptyFieldAttribute : meta::Attribute
{};

} // namespace serialization

//-----------------------------------------------------------------------------
namespace serialization_internal {

/**
	Implements serialization::SerializationException
*/
class SerializationExceptionImpl final : public ExceptionImpl<serialization::SerializationException>
{
	using ExceptionBase = ExceptionImpl<serialization::SerializationException>;
	DECLARE_CLASS_BASE(ExceptionBase)
public:
	using ExceptionBase::ExceptionImpl;
};

/**
	Implements serialization::RequiredFieldMissedException
*/
class RequiredFieldMissedExceptionImpl final : public ExceptionImpl<serialization::RequiredFieldMissedException>
{
	using ExceptionBase = ExceptionImpl<serialization::RequiredFieldMissedException>;
	DECLARE_CLASS_BASE(ExceptionBase)
public:

	RequiredFieldMissedExceptionImpl(const diagnostics::SourceInfo& sourceInfo_, std::string_view typeName_, std::string_view fieldName_)
		: ExceptionBase(sourceInfo_, strfmt(L"Required field ({0}.{1}) missed", typeName_, fieldName_))
		, m_typeName(typeName_)
		, m_fieldName(fieldName_)
	{}

	std::string typeName() const override {
		return m_typeName;
	}

	std::string fieldName() const override {
		return m_fieldName;
	}

private:
	std::string m_typeName;
	std::string m_fieldName;
};

/**
	Implements serialization::RequiredFieldMissedException
*/
class InvalidTypeExceptionImpl  : public ExceptionImpl<serialization::InvalidTypeException>
{
	using ExceptionBase = ExceptionImpl<serialization::InvalidTypeException>;
	DECLARE_CLASS_BASE(ExceptionBase)
public:

	InvalidTypeExceptionImpl(const diagnostics::SourceInfo& sourceInfo_, std::string_view expectedTypeName_, std::string_view actualTypeName_)
		: ExceptionBase(sourceInfo_, strfmt(L"Expected type(category):({0}), but:({1})", expectedTypeName_, actualTypeName_))
		, m_expectedTypeName(expectedTypeName_)
		, m_actualTypeName(actualTypeName_)
	{}

	std::string expectedTypeName() const override {
		return m_expectedTypeName;
	}

	std::string actualTypeName() const override {
		return m_actualTypeName;
	}

private:
	std::string m_expectedTypeName;
	std::string m_actualTypeName;
};

/**
	Implements serialization::NumericOverflowExcepion
*/
class NumericOverflowExcepionImpl final : public ExceptionImpl<serialization::NumericOverflowExcepion>
{
	using ExceptionBase = ExceptionImpl<serialization::NumericOverflowExcepion>;
	DECLARE_CLASS_BASE(ExceptionBase)
public:
	using ExceptionBase::ExceptionImpl;
};

/**
	Implements serialization::EndOfStreamException
*/
class EndOfStreamExceptionImpl final : public ExceptionImpl<serialization::EndOfStreamException>
{
	using ExceptionBase = ExceptionImpl<serialization::EndOfStreamException>;
	DECLARE_CLASS_BASE(ExceptionBase)
public:
	using ExceptionBase::ExceptionImpl;

	EndOfStreamExceptionImpl(const diagnostics::SourceInfo& source): ExceptionBase(source, L"Unexpected end of stream")
	{}


};

}// namespace serialization_internal


} // namespace cold;