#pragma once
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/preprocessor.h>
#include <exception>


namespace cold::internal {

struct NoExceptGuard
{
	const wchar_t* const func;
	const wchar_t* const source;
	const int line;

	NoExceptGuard(const wchar_t* func_, const wchar_t* source_, int line_): func(func_), source(source_), line(line_)
	{}

	template<typename F>
	inline friend void operator + (NoExceptGuard guard, F f) noexcept
	{
		using namespace cold::diagnostics;

		try
		{
			f();
		}
		catch (const std::exception& exception)
		{
			auto message = strfmt(L"{0}", exception.what());
			RuntimeCheck::raiseFailure(SourceInfo{guard.func, guard.source, guard.line}, nullptr, L"NOEXCEPT GUARD", message);
		}
		catch (...)
		{
			RuntimeCheck::raiseFailure(SourceInfo{guard.func, guard.source, guard.line}, nullptr, L"NOEXCEPT GUARD", L"Non type exception");
		}
	}
};

}

#define NOEXCEPT_Guard cold::internal::NoExceptGuard{WFUNCTION, WFILE, __LINE__} + [&]()


#if !defined(NDEBUG)
#define DEBUG_NOEXCEPT_Guard cold::internal::NoExceptGuard{WFUNCTION, WFILE, __LINE__} + [&]()
#else
#define DEBUG_NOEXCEPT_Guard
#endif

