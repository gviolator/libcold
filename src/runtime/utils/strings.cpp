#include "pch.h"
#include "cold/utils/strings.h"
#include "cold/utils/stringconv.h"

#include <unicode/unistr.h>


static_assert(sizeof(wchar_t) == sizeof(uint16_t), "Invalid wide character size");

// using namespace cold::strings_internal;
using namespace icu;

namespace cold::strings_internal {

namespace {


template<typename T>
std::basic_string_view<T> splitNext_(std::basic_string_view<T> str, std::basic_string_view<T> current, std::basic_string_view<T> separators)
{
	using StringView = std::basic_string_view<T>;

	if (str.empty())
	{
		return {};
	}

	size_t offset = current.empty() ? 0 : (current.data() - str.data()) + current.size();

	if (offset == str.size())
	{
		return {};
	}

	do
	{
		//DEBUG_CHECK(offset < str.size())

		const auto pos = str.find_first_of(separators, offset);

		if (pos == StringView::npos)
		{
			break;
		}

		if (pos != offset)
		{
			return StringView(str.data() + offset, pos - offset);
		}

		++offset;
	}
	while (offset < str.size());

	return offset == str.size() ? StringView{} : StringView(str.data() + offset, str.size() - offset);
}

template<typename T>
std::basic_string_view<T> trimEnd_(std::basic_string_view<T> str)
{
	constexpr T Space = TYPED_STR(T, ' ');

	size_t spaces = 0;

	for (auto r = str.rbegin(); r != str.rend(); ++r)
	{
		if (*r != Space)
		{
			break;
		}
		++spaces;
	}

	return spaces == 0 ? str : std::basic_string_view<T>{str.data(), str.length() - spaces};
}

template<typename T>
std::basic_string_view<T> trimStart_(std::basic_string_view<T> str)
{
	constexpr T Space = TYPED_STR(T, ' ');

	auto iter = str.begin();

	for (; iter != str.end(); ++iter)
	{
		if (*iter != Space)
		{
			break;
		}
	}

	const auto offset = std::distance(str.begin(), iter);

	//DEBUG_CHECK(offset >= 0)

	if (offset == str.length())
	{
		return {};
	}

	return offset == 0 ? str : std::basic_string_view<T>{str.data() + offset, (str.length() - offset)};
}

template<typename T>
std::basic_string_view<T> trim_(std::basic_string_view<T> str)
{
	return trimStart_(trimEnd_(str));
}


template<typename T>
bool endWith_(std::basic_string_view<T> str, std::basic_string_view<T> value)
{
	const auto pos = str.rfind(value);
	return (pos != std::basic_string_view<T>::npos) && (str.length() - pos) == value.length();
}

template<typename T>
bool startWith_(std::basic_string_view<T> str, std::basic_string_view<T> value)
{
	return str.find(value) == 0;
}


} // namespace 


//-----------------------------------------------------------------------------
std::string_view splitNext(std::string_view str, std::string_view current, std::string_view separators)
{
	return splitNext_(str, current, separators);
}


std::wstring_view splitNext(std::wstring_view str, std::wstring_view current, std::wstring_view separators)
{
	return splitNext_(str, current, separators);
}

} // cold::strings_internal

namespace cold::strings {

//-----------------------------------------------------------------------------
std::string_view trimEnd(std::string_view str)
{
	return strings_internal::trimEnd_(str);
}


std::wstring_view trimEnd(std::wstring_view str)
{
	return strings_internal::trimEnd_(str);
}


std::string_view trimStart(std::string_view str)
{
	return strings_internal::trimStart_(str);
}


std::wstring_view trimStart(std::wstring_view str)
{
	return strings_internal::trimStart_(str);
}


std::string_view trim(std::string_view str)
{
	return strings_internal::trim_(str);
}


std::wstring_view trim(std::wstring_view str)
{
	return strings_internal::trim_(str);
}


bool endWith(std::string_view str, std::string_view value)
{
	return strings_internal::endWith_(str, value);
}

bool endWith(std::wstring_view str, std::wstring_view value)
{
	return strings_internal::endWith_(str, value);
}

bool startWith(std::string_view str, std::string_view value)
{
	return strings_internal::startWith_(str, value);
}

bool startWith(std::wstring_view str, std::wstring_view value)
{
	return strings_internal::startWith_(str, value);
}



namespace {

inline std::wstring wstringFromUnicodeString(const icu::UnicodeString& uStr)
{
	std::wstring wstr;
	wstr.resize(uStr.length());
	uStr.extract(0, uStr.length(), icu::Char16Ptr{wstr.data()});

	return wstr;

}

} // namespace


std::string toUtf8(std::wstring_view wideStr)
{
	constexpr UBool IsNullTerminated = false;

	// using ConstCharPtr allow to avoid copying data into internal UnicodeString's buffer, so called buffer aliasing.
	const UnicodeString uStr{IsNullTerminated, ConstChar16Ptr{wideStr.data()}, static_cast<int32_t>(wideStr.length())};

	std::string bytes;
	bytes.reserve(wideStr.length());
	uStr.toUTF8String(bytes);

	return bytes;
}

std::wstring wstringFromUtf8(std::string_view str)
{
	const UnicodeString uStr = UnicodeString::fromUTF8(StringPiece{str.data(), static_cast<int32_t>(str.size())});
	return wstringFromUnicodeString(uStr);
}

std::wstring wstringFromUtf8Unescape(std::string_view str)
{
	using namespace icu;

	const UnicodeString uStr = UnicodeString::fromUTF8(StringPiece{str.data(), static_cast<int32_t>(str.size())});
	return wstringFromUnicodeString(uStr.unescape());
}



} // namespace cold::strings