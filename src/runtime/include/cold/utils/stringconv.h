#pragma once
#include <cold/runtime/runtimeexport.h>

#include <string>
#include <string_view>


namespace cold {


[[nodiscard]] std::string RUNTIME_EXPORT wstringToUtf8(std::wstring_view);

[[nodiscard]] std::wstring RUNTIME_EXPORT utf8ToWString(std::string_view);

// [[nodiscard]] std::wstring RUNTIME_EXPORT wstringFromUtf8Unescape(std::string_view);

}
