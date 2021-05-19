#pragma once
#include <cold/runtime/runtimeexport.h>

#include <string>
#include <string_view>


namespace cold::strings {

[[nodiscard]] std::string RUNTIME_EXPORT toUtf8(std::wstring_view);

[[nodiscard]] std::wstring RUNTIME_EXPORT wstringFromUtf8(std::string_view);

[[nodiscard]] std::wstring RUNTIME_EXPORT wstringFromUtf8Unescape(std::string_view);

}
