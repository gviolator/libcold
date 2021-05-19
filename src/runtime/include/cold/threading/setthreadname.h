#pragma once
#include <cold/runtime/runtimeexport.h>


namespace cold::threading {

RUNTIME_EXPORT void setCurrentThreadName(const char*) noexcept;

}
