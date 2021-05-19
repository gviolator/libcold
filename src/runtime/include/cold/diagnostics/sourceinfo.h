#pragma once

#include <optional>
#include <string_view>

namespace cold::diagnostics {

/**
*/
struct SourceInfo
{
	const std::string_view functionName;
	const std::string_view filePath;
	const std::optional<unsigned> line;


	SourceInfo(std::string_view function_, std::string_view filePath_, std::optional<unsigned> line_  = std::nullopt): functionName(function_), filePath(filePath_), line(line_)
	{}

	explicit operator bool () const
	{
		return !functionName.empty() || !filePath.empty();
	}
};

}

#define INLINED_SOURCE_INFO ::cold::diagnostics::SourceInfo{std::string_view{__FUNCTION__}, std::string_view{__FILE__}, static_cast<unsigned>(__LINE__)}
