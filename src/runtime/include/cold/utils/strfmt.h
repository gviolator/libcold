#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/utils/strings.h>
#include <cold/utils/tostring.h>
#include <cold/utils/formatters.h>

#include <array>
#include <tuple>

namespace cold {
namespace cold_internal {

template<typename T>
using StreamArgumentWriterFunc__ = void (*) (std::basic_ostream<T>&, const void*) noexcept;

template<typename T>
using FormatArgumentEntry__ = std::tuple<StreamArgumentWriterFunc__<T>, const void*>;


RUNTIME_EXPORT std::string formatImpl__(std::string_view, const FormatArgumentEntry__<char>* , size_t);

RUNTIME_EXPORT std::wstring formatImpl__(std::wstring_view, const FormatArgumentEntry__<wchar_t>* , size_t);

template<typename T, typename C>
void formatValue__(std::basic_ostream<C>& stream, const void* valuePtr) noexcept
{
	const T& value = *reinterpret_cast<const T*>(valuePtr);

	if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, const wchar_t*>)
	{
		if (value == nullptr)
		{
			return;
		}
	}

	cold::formatters::formatValue(stream, value);
}


template<typename C>
inline std::basic_string<C> format__(std::basic_string_view<C> text)
{
	return std::basic_string<C>{text};
}

template<typename C, typename ... Args>
inline std::basic_string<C> format__(std::basic_string_view<C> text, const Args& ... args)
{
	using StreamArgumentWriterFunc = StreamArgumentWriterFunc__<C>;
	using FormatArgumentEntry = FormatArgumentEntry__<C>;

	if (text.empty())
	{
		return {};
	}

	const std::array<FormatArgumentEntry, sizeof ... (args)> arguments =
	{
		FormatArgumentEntry {&formatValue__<Args,C>, reinterpret_cast<const void*>(&args)} ...
	};

	return formatImpl__(text, arguments.data(), arguments.size());
}

} // namespace cold_internal


/**
*/
template<typename ... Args>
inline std::string strfmt(std::string_view text, const Args& ... args)
{
	return cold_internal::format__(text, args ...);
}

/**
*/
template<typename ... Args>
std::wstring strfmt(std::wstring_view text, const Args& ... args)
{
	return cold_internal::format__(text, args ...);
}

}
