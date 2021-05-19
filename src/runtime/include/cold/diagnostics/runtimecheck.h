#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/diagnostics/diaginternal.h>
#include <cold/diagnostics/sourceinfo.h>
#include <cold/utils/functor.h>

namespace cold::diagnostics {

/**
*/
struct RuntimeCheck
{
	using FailureHandler = Functor<void (const SourceInfo&, const wchar_t* moduleName, const wchar_t* expression, const wchar_t* message)>;


	RUNTIME_EXPORT static void setFailureHandler(FailureHandler::UniquePtr, FailureHandler::UniquePtr* prev = nullptr);

	RUNTIME_EXPORT static void raiseFailure(SourceInfo source, const wchar_t* moduleName, const wchar_t* expression, std::wstring message);
};

} // namespace cold::diagnostics


namespace cold::diagnostics_internal {

template<typename T>
inline bool safeBoolean(const T& value)
{
	using ValueType = std::decay_t<T>;

	static_assert(!(
		std::is_same_v<ValueType, const wchar_t*> ||
		std::is_same_v<ValueType, const char*>), "Unsafe condition type");

	return static_cast<bool>(value);
}

} // namespace cold::diagnostics_internal




//
#define RUNTIME_FAILURE(...) \
::cold::diagnostics::RuntimeCheck::raiseFailure(INLINED_SOURCE_INFO, nullptr, L"RUNTIME_FAILURE", ::cold::diagnostics_internal::diagWStringMessage(__VA_ARGS__)); \


#define RUNTIME_CHECK(expression, ...) \
if (!::cold::diagnostics_internal::safeBoolean(expression)) { \
	::cold::diagnostics::RuntimeCheck::raiseFailure(INLINED_SOURCE_INFO, nullptr, L ## #expression, ::cold::diagnostics_internal::diagWStringMessage(__VA_ARGS__)); \
}\


#if defined(NDEBUG) && !defined(CHECK_ALWAYS)
#define DEBUG_CHECK(cond, ...)
#else
#define DEBUG_CHECK(expression, ...) RUNTIME_CHECK(expression, __VA_ARGS__)
#endif
