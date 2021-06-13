#include "pch.h"
#include "cold/utils/stringconv.h"
#include "cold/memory/rtstack.h"


namespace cold {

namespace {



inline std::wstring multiByteToWString(int encoding, std::string_view mbStr)
{
	if (mbStr.empty())
	{
		return {};
	}

	const int len = MultiByteToWideChar(encoding, 0, mbStr.data(), static_cast<int>(mbStr.length()), nullptr, 0);
	if (len <= 0)
	{
		return {};
	}

	rtstack();

	std::pmr::vector<wchar_t> buffer(static_cast<size_t>(len), cold::rtStackMemoryResource());
	MultiByteToWideChar(encoding, 0, mbStr.data(), static_cast<int>(mbStr.length()), buffer.data(), len);

	return {buffer.data(), buffer.size()};
}


inline std::string wideToMultiBytesString(int encoding, std::wstring_view str)
{
	if (str.empty())
	{
		return {};
	}

	const int len = WideCharToMultiByte(encoding, 0, str.data(), static_cast<int>(str.length()), nullptr, 0, nullptr, nullptr);
	if (len <= 0)
	{
		return {};
	}

	rtstack();

	std::pmr::vector<char> buffer(static_cast<size_t>(len), cold::rtStackMemoryResource());
	WideCharToMultiByte(encoding, 0, str.data(), static_cast<int>(str.length()), buffer.data(), len, nullptr, nullptr);

	return {buffer.data(), buffer.size()};
}

} // namespace



std::string wstringToUtf8(std::wstring_view wideStr)
{
	return wideToMultiBytesString(CP_UTF8, wideStr);
}

std::wstring utf8ToWString(std::string_view str)
{
	return multiByteToWString(CP_UTF8, str);
}

//std::wstring wstringFromUtf8Unescape(std::string_view str)
//{
//	using namespace icu;
//
//	const UnicodeString uStr = UnicodeString::fromUTF8(StringPiece{str.data(), static_cast<int32_t>(str.size())});
//	return wstringFromUnicodeString(uStr.unescape());
//}

}
