#pragma once
#include <cold/meta/attribute.h>
#include <cold/meta/classinfo.h>
#include <cold/utils/preprocessor.h>
#include <cold/utils/stringconv.h>

#include <exception>
#include <string>
#include <type_traits>

//#pragma warning(push)
//#pragma warning(disable: 4250 4251 4275)

namespace cold {


struct ExceptionImplementationAttribute : meta::Attribute
{};

template<typename T>
struct ExceptionImplementation : meta::AttributeValue<ExceptionImplementationAttribute>
{
	using type = T;
};


class ABSTRACT_TYPE Exception : public virtual std::exception
{
	DECLARE_CLASS_BASE(std::exception)
public:

	virtual const wchar_t* message() const = 0;
	virtual const wchar_t* diagMessage() const = 0;
	virtual diagnostics::SourceInfo source() const = 0;
	virtual unsigned flags() const = 0;

protected:
	virtual ~Exception() = default;
};

/**
*/
template<typename E>
class ExceptionImpl : public E
{
	static_assert(std::is_base_of_v<Exception, E>, "Type must inherit cold::Exception");
	DECLARE_CLASS_BASE(E)

public:
	ExceptionImpl(const diagnostics::SourceInfo& sourceInfo_, std::wstring message_ , unsigned flags_ = 0)
		: m_sourceInfo(sourceInfo_)
		, m_message(std::move(message_))
		, m_flags(flags_)
	{}

	ExceptionImpl(const diagnostics::SourceInfo& sourceInfo_, std::string_view message_,  unsigned flags_ = 0)
		: ExceptionImpl(sourceInfo_, cold::utf8ToWString(message_), flags_)
	{}

	const char* what() const override
	{
		if (!m_what)
		{
			m_what = cold::wstringToUtf8(m_message);
		}
		return m_what->c_str();
	}

	const wchar_t* message() const override
	{
		return m_message.c_str();
	}

	const wchar_t* diagMessage() const override
	{
		if (!m_sourceInfo)
		{
			return m_message.c_str();
		}

		if (!m_diagMessage)
		{
			const unsigned line = m_sourceInfo.line ? *m_sourceInfo.line : 0;
			m_diagMessage = strfmt(L"{0}({1}):{2}", m_sourceInfo.filePath, line, m_message);
		}

		return m_diagMessage->c_str();
	}

	diagnostics::SourceInfo source() const  override {
		return m_sourceInfo;
	}

	unsigned flags() const  override {
		return m_flags;
	}

private:

	const diagnostics::SourceInfo m_sourceInfo;
	const std::wstring m_message;
	const unsigned m_flags;
	mutable std::optional<std::wstring> m_diagMessage;
	mutable std::optional<std::string> m_what;
};


namespace internal {

class DefaultException : public ExceptionImpl<Exception>
{
	CLASS_INFO(
		CLASS_BASE(ExceptionImpl<Exception>)
	)
public:
	using ExceptionImpl<Exception>::ExceptionImpl;

};


template<typename T = cold::Exception, bool HasImplementation = meta::AttributeDefined<T, ExceptionImplementationAttribute>>
struct ExceptionImplType__;

template<>
struct ExceptionImplType__<cold::Exception, false>
{
	using type = DefaultException;
};

template<typename T>
struct ExceptionImplType__<T, false>
{
	using type = T;
};

template<typename T>
struct ExceptionImplType__<T, true>
{
	using type = typename meta::AttributeValueType<T, ExceptionImplementationAttribute>::type;
};

} // namespace internal


template<typename T>
using ExceptionImplType = typename internal::ExceptionImplType__<T>::type;


} // namespace cold


#define Excpt(ExceptionType, ...) ::cold::ExceptionImplType<ExceptionType>(INLINED_SOURCE_INFO, __VA_ARGS__)
#define Excpt_(message, ...) Excpt(::cold::Exception, ::cold::diagnostics_internal::diagWStringMessage(message, __VA_ARGS__))

#define MAKE_Excpt(ExceptionType, ...) std::make_exception_ptr(Excpt(ExceptionType, __VA_ARGS__))
#define MAKE_Excpt_(message, ...) std::make_exception_ptr(Excpt_(message, __VA_ARGS__))

#define DECLARE_ABSTRACT_EXCEPTION(Base, ImplType) \
	CLASS_INFO(\
		CLASS_BASE(Base),\
		cold::ExceptionImplementation<ImplType>{}\
	)\
