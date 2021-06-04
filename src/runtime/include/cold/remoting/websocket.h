#pragma once

#include <cold/memory/bytesbuffer.h>


namespace cold::websocket {

/// <summary>
///
/// </summary>
enum class FrameType
{
	ContinuePayload = 0x00,
	Utf8Text = 0x01,
	Binary = 0x02,
	CloseConnection = 0x08,
	Ping = 0x09,
	Pong = 0x0A,
	Unknown = 0xFF
};


using MessageType = FrameType;


/// <summary>
///
/// </summary>
struct FrameHeader
{
	const FrameType type;
	const bool isFinal;
	const uint64_t payloadLength;
	const uint32_t maskingKey;
	const size_t headerSize;

	FrameHeader(FrameType type_, bool isFinal_, uint64_t payloadLength_, uint32_t maskingKey_, size_t headerSize_);
};


struct MessageFrame
{
	BytesBuffer payload;
	FrameType type = FrameType::Unknown;
	uint32_t key = 0;

	explicit operator bool () const
	{
		return static_cast<bool>(payload);
	}
};



/// <summary>
///
/// </summary>
class MessageReader final
{
public:

	MessageReader();

	void addBuffer(BufferView&&);

	MessageFrame getMessageFrame();

	BytesBuffer& buffer();

	std::byte* popBytes(size_t count, BytesBuffer* dest = nullptr);

private:

	BytesBuffer m_buffer;
	MessageFrame m_multiFrameMessage;
	bool m_streaming = false;
};



constexpr size_t FrameHeaderMaxSize = 14;

using FrameHeaderBytes = std::byte[FrameHeaderMaxSize];


std::optional<FrameHeader> parseFrameHeader(const std::byte* buffer, size_t size);

std::tuple<size_t, uint32_t> initializeFrameHeaderBytes(FrameHeaderBytes&, FrameType type, bool masked, uint64_t payloadLength, bool finalize = true, uint32_t overrideMask = 0);

std::string createSecWebSocketKeyValue();

std::string createSecWebSocketAcceptValue(std::string_view secWebSocketKey);

void applyMaskingKey(uint32_t maskingKey, std::byte*, size_t size);

}
