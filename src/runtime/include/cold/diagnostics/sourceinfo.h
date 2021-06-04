#pragma once
#include <cold/utils/preprocessor.h>
#include <optional>
#include <string_view>

namespace cold::diagnostics {

/**
*/
struct SourceInfo
{
	const std::wstring_view functionName;
	const std::wstring_view filePath;
	const std::optional<unsigned> line;


	SourceInfo(std::wstring_view function_, std::wstring_view filePath_, std::optional<unsigned> line_  = std::nullopt): functionName(function_), filePath(filePath_), line(line_)
	{}

	explicit operator bool () const
	{
		return !functionName.empty() || !filePath.empty();
	}
};

}

#define INLINED_SOURCE_INFO ::cold::diagnostics::SourceInfo{std::wstring_view{WFUNCTION}, std::wstring_view{WFILE}, static_cast<unsigned>(__LINE__)}
