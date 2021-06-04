#include "pch.h"
#include "runtimecheckinternal.h"
#include "cold/utils/strfmt.h"


namespace cold::diagnostics {


RuntimeCheckInternal& RuntimeCheckInternal::instance()
{
	static RuntimeCheckInternal instance;

	return instance;
}


const void* RuntimeCheckInternal::setFailureHandler(FailureHandler::UniquePtr handler, FailureHandler::UniquePtr* prev)
{
	if (prev)
	{
		*prev = std::move(m_failureHandler);
	}

	m_failureHandler = std::move(handler);

	return reinterpret_cast<const void*>(m_failureHandler.get());
}


void RuntimeCheckInternal::removeFailureHandler(const void*)
{
	m_failureHandler.reset();
}


void RuntimeCheckInternal::raiseFailure(SourceInfo source, const wchar_t* moduleName, const wchar_t* expression, std::wstring message)
{
	if (m_failureHandler)
	{
		m_failureHandler->invoke(source, moduleName, expression, message.c_str());
		return;
	}

	const wchar_t* unknown = L"unknown";

	const auto failureMessage = strfmt(L"\nASSERTION FAILURE: {0}\n\nCallee:({1}) in module:({2})\n{3}({4}):{5}\n",
		expression,
		source.functionName,
		moduleName ? unknown : moduleName,
		source.filePath,
		source.line ? *source.line : 0,
		message);

	if (IsDebuggerPresent()) {
		OutputDebugStringW(failureMessage.c_str());
		DebugBreak();
	}

	MessageBoxW(nullptr, (failureMessage).c_str(), L"FAILURE", MB_OK | MB_ICONERROR);

	std::terminate();
}



}
