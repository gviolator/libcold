#pragma once
#include <cold/utils/strfmt.h>
#include <cold/utils/stringconv.h>


namespace cold::diagnostics_internal {


inline std::wstring diagWStringMessage() {
	return L"";
}

/// <summary>
///
/// </summary>
template<typename ... T>
std::wstring diagWStringMessage(std::wstring_view message, const T& ... args)
{
	return cold::strfmt(message, args ...);
}


template<typename ... T>
std::wstring diagWStringMessage(std::string_view message, const T& ... args)
{
	const std::wstring wstrMessage = cold::utf8ToWString(message);
	return cold::strfmt(wstrMessage, args ...);
}

}
