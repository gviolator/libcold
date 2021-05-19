#pragma once
#include <cold/diagnostics/runtimecheck.h>


namespace cold::diagnostics {


class RuntimeCheckInternal
{
public:

	using FailureHandler = RuntimeCheck::FailureHandler;

	static RuntimeCheckInternal& instance();


	const void* setFailureHandler(FailureHandler::UniquePtr, FailureHandler::UniquePtr*);

	void removeFailureHandler(const void*);

	void raiseFailure(SourceInfo source, const wchar_t* moduleName, const wchar_t* expression, std::wstring message);

private:

	FailureHandler::UniquePtr m_failureHandler;
};


}


