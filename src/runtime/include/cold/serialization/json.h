#pragma once
#include <cold/diagnostics/exception.h>
#include <cold/diagnostics/runtimecheck.h>
#if ! __has_include(<rapidjson/stringbuffer.h>)
#error rapid json dependency not used for current project
#endif

#include <cold/meta/classinfo.h>
#include <cold/serialization/serialization.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wambiguous-reversed-operator"
#endif

#include <rapidjson/document.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <array>

namespace cold::serialization::json {

class JsonParseExceptionImpl;


class JsonParseException : public serialization::SerializationException
{
	DECLARE_ABSTRACT_EXCEPTION(serialization::SerializationException, JsonParseExceptionImpl)
};


using DefaultEncoding = rapidjson::UTF8<char>;
using JsonDocument = rapidjson::GenericDocument<DefaultEncoding>;
using JsonValue = rapidjson::GenericValue<DefaultEncoding>;



/**

*/
//template<typename Encoding = rapidjson::UTF8<>>
class ReaderStream
{
public:
	using Ch = typename DefaultEncoding::Ch;

	static constexpr Ch NoChar = Ch {0};

	ReaderStream(io::Reader& reader_): m_reader(reader_){
		this->fillBuffer();
	}

	Ch Peek() const {
		return (m_inBufferPos == m_avail) ? NoChar : m_buffer[m_inBufferPos];
	}

	Ch Take() {
		if (m_inBufferPos == m_avail) {
			return NoChar;
		}

		++m_offset;
		const Ch ch = m_buffer[m_inBufferPos];
		if (++m_inBufferPos == m_avail) {
			this->fillBuffer();
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

	static constexpr size_t PrefetchBufferSize = 16;
	using Buffer = std::array<Ch, PrefetchBufferSize>;

	void fillBuffer() {
		DEBUG_CHECK(m_inBufferPos == m_avail)

		std::byte* const ptr = reinterpret_cast<std::byte*>(m_buffer.data());
		const size_t readSize = m_buffer.size() * sizeof(Ch);
		m_avail = m_reader.read(ptr, readSize);
		DEBUG_CHECK(m_avail % sizeof(Ch) == 0)
		m_avail = m_avail / sizeof(Ch);
		m_inBufferPos = 0;
	}

	io::Reader& m_reader;
	Buffer m_buffer;
	size_t m_offset = 0;
	size_t m_avail = 0;
	size_t m_inBufferPos = 0;
};


Result<> parse(JsonDocument& document, ReaderStream& stream);

}
