#pragma once

#include <cold/runtime/runtimeexport.h>
#include <cold/utils/tostring.h>

#if __has_include(<QString>)
#include <QString>
#define FORMATTER_SUPPORT_QSTRING
#endif


#include <array>
// #include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace cold::formatters {

template<typename T>
inline constexpr bool StringFormatable =
	std::is_assignable_v<std::string_view, T> ||
	std::is_assignable_v<std::wstring_view, T> 

#ifdef FORMATTER_SUPPORT_QSTRING
	|| std::is_same_v<QString, T>
	|| std::is_same_v<QLatin1String, T>
#endif
	;


template<typename C, typename T>
requires StringRepresentableT<T, C>
inline void formatValue(std::basic_ostream<C>& stream, const T& value) {
	stream << toStringT<C>(value);
}


template<typename C, typename T,
	std::enable_if_t<!StringFormatable<T>, int> = 0
>
inline void formatValue(std::basic_ostream<C>& stream, const T& value)
{
	//static_assert(!std::is_same_v<T, std::filesystem::path>, "Implicit std::filesystem::path formatting is prohibited. Use string()/wstring() or generic_string()/generic_wstring()");
	stream << value;
}

inline void formatValue(std::basic_ostream<char>& stream, std::string_view str)
{
	stream << str;
}

inline void formatValue(std::basic_ostream<wchar_t>& stream, std::wstring_view str)
{
	stream << str;
}

void RUNTIME_EXPORT formatValue(std::basic_ostream<char>& stream, std::wstring_view);

void RUNTIME_EXPORT formatValue(std::basic_ostream<wchar_t>& stream, std::string_view);


#ifdef FORMATTER_SUPPORT_QSTRING

inline void formatValue(std::basic_ostream<char>& stream, const QString& qstr)
{
	stream << qstr.toStdString();
}


inline void formatValue(std::basic_ostream<wchar_t>& stream, const QString& qstr)
{
	stream << qstr.toStdWString();
}

#endif


inline void formatValue(std::basic_ostream<char>& stream, const char* str) {
	if (str) {
		stream << str;
	}
}


inline void formatValue(std::basic_ostream<wchar_t>& stream, const char* str) {
	if (str) {
		formatValue(stream, std::string_view{str});
	}
}


/// <summary>
/// Optional values formatter
/// </summary>
template<typename C, typename T>
inline void formatValue(std::basic_ostream<C>& stream, const std::optional<T>& value) {
	if (value) {
		formatValue(stream, *value);
	}
	else {
		// stream << TYPED_STRING(C, "[null]");
	}
}


template<typename C>
inline void formatValue(std::basic_ostream<C>& stream, const std::type_info& value) {
	formatValue(stream, value.name());
}


} // namespace cold::formatters


#ifdef FORMATTER_SUPPORT_QSTRING
#undef FORMATTER_SUPPORT_QSTRING
#endif


