#pragma once
#include <cold/utils/format.h>
#include <cold/utils/stringconv.h>


namespace cold::diagnostics_internal {


inline std::wstring diagWStringMessage() {
	return L"";
}

/// <summary>
///
/// </summary>
template<typename ... T>
inline std::wstring diagWStringMessage(std::wstring_view message, const T& ... args) {
	return cold::format(message, args ...);
}


template<typename ... T>
inline std::wstring diagWStringMessage(std::string_view message, const T& ... args) {
	const std::wstring wstrMessage = strings::wstringFromUtf8(message);
	return cold::format(wstrMessage, args ...);
}

}
