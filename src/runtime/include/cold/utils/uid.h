#pragma once

#include <cold/runtime/runtimeexport.h>
#include <cold/utils/result.h>

#if __has_include(<QMetaType>)
#include <QMetaType>
#endif

#ifdef _WIN32
#include <guiddef.h>
#else
#error "Unspoorted platform"
#endif

#include <optional>
#include <string>
#include <string_view>

namespace cold {

/**

*/
struct RUNTIME_EXPORT Uid
{
#ifdef _WIN32
		GUID data__;
#else

#endif

	/**

	*/
	static Uid generate();

	/**
	*/
	static std::optional<Uid> parse(std::string_view);

	/**
	*/
	static std::optional<Uid> parse(std::wstring_view);

	Uid() noexcept;

	explicit operator bool () const noexcept;

private:

	static std::string toString__(Uid) noexcept;

	static std::wstring toWString__(Uid) noexcept;

	friend std::string getPrimitiveValue(Uid uid)
	{
		return toString(uid);
	}

	friend Result<> setPrimitiveValue(Uid& uid, std::string_view str)
	{
		if (const auto parseResult = Uid::parse(str); parseResult)
		{
			uid = *parseResult;
			return success;
		}

		return MAKE_Excpt_(L"Invalid uid value:{0}", str);
	}


	/**
	*/
	friend std::string toString(Uid uid) noexcept
	{
		return Uid::toString__(uid);
	}

	/**
	*/
	friend std::wstring toWString(Uid uid) noexcept
	{
		return Uid::toWString__(uid);
	}

	template <typename Traits>
	friend std::basic_ostream<char ,Traits>&  operator << (std::basic_ostream<char, Traits>& stream, Uid uid)
	{
		stream << toString__(uid);
		return stream;
	}

	template <typename Traits>
	friend std::basic_ostream<wchar_t ,Traits>&  operator<< (std::basic_ostream<wchar_t, Traits>& stream, Uid uid)
	{
		stream << toWString__(uid);
		return stream;
	}
};

bool RUNTIME_EXPORT operator < (Uid, Uid) noexcept;
bool RUNTIME_EXPORT operator == (Uid, Uid) noexcept;
bool RUNTIME_EXPORT operator != (Uid, Uid) noexcept;

#if 0

namespace Internal {

/// <summary>
///
/// </summary>
struct UidValueProxy
{
	static std::string getValue(const Uid& uid)
	{
		return uid.toString();
	}

	static void setValue(Uid& uid, const std::string& strValue)
	{
		const auto parsedUid = Uid::parse(strValue);

		if (!parsedUid)
		{
			throw std::exception(format("(%1) is not valid Uid/Guid value", strValue).c_str());
		}

		uid = *parsedUid;
	}
};


}} // namespace cold::Internal


ATTRIBUTE_VALUE(cold::Uid, cold::meta::ValueProxyAttribute, cold::meta::ValueProxy<cold::Internal::UidValueProxy>{})

ATTRIBUTE_(cold::Uid, cold::meta::ClassNameAttribute, "guid")

PRIMITIVE_VALUE_PROXY(cold::Uid, std::string)

#endif


#if __has_include(<QMetaType>)
Q_DECLARE_METATYPE(cold::Uid)
#endif

}