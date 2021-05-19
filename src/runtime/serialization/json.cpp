#include "pch.h"
#include "cold/serialization/json.h"
#include "cold/serialization/serialization.h"
#include "cold/diagnostics/exception.h"
#include "cold/diagnostics/sourceinfo.h"


using namespace cold::serialization;

namespace cold::serialization::json {

Result<> parse(JsonDocument& document, ReaderStream& stream) {
	
	constexpr unsigned ParseFlags = rapidjson::kParseCommentsFlag | rapidjson::kParseStopWhenDoneFlag;

	document.ParseStream<ParseFlags>(stream);

	const auto error = document.GetParseError();

	switch (error)
	{
	case rapidjson::kParseErrorNone:
	{
		break;
	}

	case rapidjson::kParseErrorDocumentEmpty:
	{
		return Excpt(EndOfStreamException);
	}

	default:
	{
		return MAKE_Excpt(SerializationException, format(L"Json error:(%1)", error));
	}
	}

	return success;
}

}
