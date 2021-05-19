#pragma once
#if ! __has_include(<rapidjson/document.h>)
#error rapid json dependency not used for current project
#endif

#include "runtimevalue.h"
#include "io/reader.h"
#include "diagnostics/runtimecheck.h"
#include "utils/result.h"

#include <rapidjson/document.h>

namespace cold::serialization::json {

using JsonDefaultEncoding = rapidjson::UTF8<char>;
using JsonDocument = rapidjson::GenericDocument<JsonDefaultEncoding, JsonDefaultEncoding>;
using JsonValue = rapidjson::GenericValue<JsonDefaultEncoding, JsonDefaultEncoding>;

/**
*/
template<typename Encoding = JsonDefaultEncoding>
class JsonReaderStream
{
public:
	using Ch = typename Encoding::Ch;

	static constexpr Ch NoChar = Ch {0};

	JsonReaderStream(io::Reader& reader_) : m_reader(reader_)
	{}

	Ch Peek() const
	{
		if (!m_char) {
			Ch chr;
			if (m_reader.read(reinterpret_cast<std::byte*>(&chr), sizeof(Ch)) == 0) {
				return NoChar;
			}

			m_char.emplace(chr);
		}

		return *m_char;
	}

	Ch Take() {
		const Ch ch = Peek();
		if (ch != NoChar) {
			++m_offset;
			m_char.reset();
		}

		return ch;
	}

	size_t Tell() const {
		return m_offset;
	}

#pragma region Methods that must be never called, but required for compilation
	Ch* PutBegin() {
		RUNTIME_FAILURE("PutBegin must not be caller for json::ReaderStream")
		return nullptr;
	}

	void Put(Ch) {
		RUNTIME_FAILURE("Put must not be caller for json::ReaderStream")
	}

	void Flush() {
		RUNTIME_FAILURE("Flush must not be caller for json::ReaderStream")
	}

	size_t PutEnd(Ch*) {
		RUNTIME_FAILURE("PutEnd must not be caller for json::ReaderStream")
		return 0;
	}
#pragma endregion

private:

	io::Reader& m_reader;
	mutable std::optional<Ch> m_char;
	size_t m_offset = 0;
};


inline Result<> parse(JsonDocument& document, io::Reader& reader)
{
	constexpr unsigned ParseFlags = rapidjson::kParseCommentsFlag | rapidjson::kParseStopWhenDoneFlag;

  JsonReaderStream<> stream{reader};

	document.ParseStream<ParseFlags>(stream);

	const auto error = document.GetParseError();

  if (error != rapidjson::kParseErrorNone)
  {
    if (error == rapidjson::kParseErrorDocumentEmpty) {
      //throw EndOfStreamException {};
    }

throw SerializationException(format("Json error:%1", error).c_str());
  }

	switch (error)
	{
	case rapidjson::kParseErrorNone:
	{
		break;
	}

	case :
	{
		//
	}

	default:
	{
		//
	}
	}

  return Result<>::success();
}




}