//The WebSocket Protocol: https://tools.ietf.org/html/rfc6455
//http://lucumr.pocoo.org/2012/9/24/websockets-101/

#include "pch.h"
#include "cold/remoting/websocket.h"
#include "cold/diagnostics/exception.h"
#include "cold/network/networkexception.h"

// crypto++
#pragma warning (push)
#pragma warning (disable: 4996)

#include "cryptopp/sha.h"
#include "cryptopp/base64.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"

#pragma warning (pop)


namespace cold::websocket {

namespace {

constexpr uint8_t Mask_0x01 = 0b00000001;
constexpr uint8_t Mask_0x0F = 0b00001111;
constexpr uint8_t Mask_0x7F = 0b01111111;

constexpr std::string_view HandshakeGuid{"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"};


struct ParseHeaderData
{
	uint8_t fin;
	uint8_t reserved[3];
	uint8_t opcode;
	uint64_t payloadLength;
	bool isMasked;
	uint32_t mask;
};


using HeaderParserStep = std::optional<size_t> (*) (const uint8_t*, size_t, ParseHeaderData&);


std::optional<size_t> parseOpCode(const uint8_t* bytes, size_t size, ParseHeaderData& header)
{
	if (size == 0)
	{
		return std::nullopt;
	}

	const uint8_t byte0 = bytes[0];

	header.fin = (byte0 >> 7) & Mask_0x01;
	header.reserved[0] = (byte0 >> 6) & Mask_0x01;
	header.reserved[1] = (byte0 >> 5) & Mask_0x01;
	header.reserved[2] = (byte0 >> 4) & Mask_0x01;
	header.opcode = byte0 & Mask_0x0F;

	return 1;
}


std::optional<size_t> parseMaskBitAndPayloadLength(const uint8_t* bytes, size_t size, ParseHeaderData& header)
{
	if (size == 0)
	{
		return std::nullopt;
	}

	const uint8_t oneByte = bytes[0];

	header.isMasked = ((oneByte >> 7) & Mask_0x01) != 0;

	const uint8_t length7bit = oneByte & Mask_0x7F;

	if (length7bit < 126)
	{
		header.payloadLength = static_cast<uint64_t>(length7bit);
		return 1;
	}

	const size_t payloadSizeBytes = length7bit == 127 ? 8 : 2;
	const size_t requireBytesCount = payloadSizeBytes + 1;

	if (size < requireBytesCount)
	{
		return std::nullopt;
	}

	if (payloadSizeBytes == 2)
	{
		const uint16_t halfWord0 = static_cast<uint16_t>(bytes[1]);
		const uint16_t halfWord1 = static_cast<uint16_t>(bytes[2]);
		const uint16_t length16 = (halfWord0 << 8) | halfWord1;

		header.payloadLength = static_cast<uint64_t>(length16);
	}
	else
	{
		DEBUG_CHECK(payloadSizeBytes == 8)

		const uint64_t byte0 = static_cast<uint64_t>(bytes[1]);
		const uint64_t byte1 = static_cast<uint64_t>(bytes[2]);
		const uint64_t byte2 = static_cast<uint64_t>(bytes[3]);
		const uint64_t byte3 = static_cast<uint64_t>(bytes[4]);
		const uint64_t byte4 = static_cast<uint64_t>(bytes[5]);
		const uint64_t byte5 = static_cast<uint64_t>(bytes[6]);
		const uint64_t byte6 = static_cast<uint64_t>(bytes[7]);
		const uint64_t byte7 = static_cast<uint64_t>(bytes[8]);

		header.payloadLength = (byte0 << 56) | (byte1 << 48) | (byte2 << 40) | (byte3 << 32) | (byte4 << 24) | (byte5 << 16) | (byte6 << 8) | byte7;
	}

	return requireBytesCount;
}


std::optional<size_t> parseMaskKey(const uint8_t* ptr, size_t size, ParseHeaderData& header)
{
	if (!header.isMasked)
	{
		return 0;
	}

	constexpr size_t KeySize = 4;
	if (size < KeySize)
	{
		return std::nullopt;
	}

	header.mask = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);

	return KeySize;
}


template<typename T>
struct Randomizer
{
	std::random_device rd;
	std::default_random_engine eng;
	std::uniform_int_distribution<T> randomizer;


	Randomizer()
		: eng(rd())
		, randomizer(1000)
	{}


	T operator ()()
	{
		return randomizer(rd);
	}
};


} // namespace

//-----------------------------------------------------------------------------

FrameHeader::FrameHeader(FrameType type_, bool isFinal_, uint64_t payloadLength_, uint32_t maskingKey_, size_t headerSize_)
	: type(type_)
	, isFinal(isFinal_)
	, payloadLength(payloadLength_)
	, maskingKey(maskingKey_)
	, headerSize(headerSize_)
{}

//-----------------------------------------------------------------------------
MessageReader::MessageReader()
{}


void MessageReader::addBuffer(BufferView&& buffer)
{
	m_buffer += std::move(buffer);
}


BytesBuffer& MessageReader::buffer()
{
	return m_buffer;
}


std::byte* MessageReader::popBytes(size_t count, BytesBuffer* dest)
{
	DEBUG_CHECK(count <= m_buffer.size())

	std::byte* const destPtr = dest ? dest->append(count) : nullptr;

	if (destPtr)
	{
		memcpy(destPtr, m_buffer.data(), count);
	}

	const size_t newSize = m_buffer.size() - count;
	if (newSize > 0)
	{
		memcpy(m_buffer.data(), m_buffer.data() + count, newSize);
	}

	m_buffer.resize(newSize);

	return destPtr;
}


MessageFrame MessageReader::getMessageFrame()
{
	do
	{
		const auto header = parseFrameHeader(m_buffer.data(), m_buffer.size());
		if (!header)
		{
			return {};
		}

		const size_t frameSize = header->headerSize + static_cast<size_t>(header->payloadLength);
		if (m_buffer.size() < frameSize)
		{
			return {};
		}

		if (header->type != FrameType::ContinuePayload)
		{
			if (m_multiFrameMessage.type != FrameType::Unknown)
			{
				throw Excpt(network::NetworkException, "Unexpected websocket frame type (ContinuePayload). Multi frame message was not initiated yet");
			}

			if (m_multiFrameMessage.payload.size() != 0)
			{
				throw Excpt(network::NetworkException, format("Expected (ContinuePayload) websocket, but: ({0})", static_cast<int>(header->type)));
			}

			m_multiFrameMessage.type = header->type;
			m_multiFrameMessage.key = header->maskingKey;
		}
		else if (m_multiFrameMessage.key != header->maskingKey)
		{
			throw Excpt(network::NetworkException, format("Invalid masking key for ws frame. expected:({0}) != actual:({1})", m_multiFrameMessage.key, header->maskingKey));
		}

		std::byte* const framePayloadPtr = m_multiFrameMessage.payload.append(header->payloadLength);
		memcpy(framePayloadPtr, m_buffer.data() + header->headerSize, header->payloadLength);
		popBytes(frameSize);

		if (header->maskingKey != 0)
		{
			applyMaskingKey(header->maskingKey, framePayloadPtr, header->payloadLength);
		}

		if (header->isFinal)
		{
			const uint32_t frameKey = this->m_streaming ? header->maskingKey : 0;
			const FrameType frameType = m_multiFrameMessage.type;
			m_multiFrameMessage.key = 0;
			m_multiFrameMessage.type = FrameType::Unknown;

			return MessageFrame { std::move(m_multiFrameMessage.payload), frameType, frameKey };
		}
	}
	while (!m_buffer.empty());

	return {};
}


//-----------------------------------------------------------------------------

std::optional<FrameHeader> parseFrameHeader(const std::byte* buffer, size_t bufferSize)
{
	HeaderParserStep parsers[] = {
		parseOpCode,
		parseMaskBitAndPayloadLength,
		parseMaskKey
	};

	auto ptr = reinterpret_cast<const uint8_t*>(buffer);
	auto size = bufferSize;

	ParseHeaderData header {0ui8, {0ui8, 0ui8, 0ui8}, 0ui8, 0ui64, false, 0};

	for (const auto parser : parsers)
	{
		if (const auto count = parser(ptr, size, header); count)
		{
			ptr = ptr + *count;
			size = size - *count;
		}
		else
		{
			return std::nullopt;
		}
	}

	const auto headerSize = bufferSize - size;

	const FrameType type = static_cast<FrameType>(header.opcode);

	const uint32_t maskingKey = header.isMasked ? header.mask : 0;


	return FrameHeader(type, header.fin != 0, header.payloadLength, maskingKey, headerSize);
}


std::tuple<size_t, uint32_t> initializeFrameHeaderBytes(FrameHeaderBytes& headerBytes, FrameType type, bool masked, uint64_t payloadLength, bool finalize, uint32_t overrideMask)
{
	uint8_t* bits = reinterpret_cast<uint8_t*>(&headerBytes[0]);

	const uint8_t opCode = static_cast<uint8_t>(type);

	*(bits++) = (finalize ? 0b10000000 : 0) | (Mask_0x0F & opCode);

	const uint8_t maskBit = masked ? 0b10000000 : 0b00000000;

	if (payloadLength <= 125)
	{
		*(bits++) = maskBit | static_cast<uint8_t>(payloadLength);
	}
	else if (payloadLength <= std::numeric_limits<uint16_t>::max())
	{
		*(bits++) = maskBit | 126ui8;

		const auto length16 = static_cast<uint16_t>(payloadLength);

		*(bits++) = static_cast<uint8_t>(length16 >> 8);
		*(bits++) = static_cast<uint8_t>(length16 & 0x00FF);
	}
	else
	{
		*(bits++) = maskBit | 127ui8;

		const auto length64 = payloadLength;

		constexpr uint64_t LastByteMask = 0xFFui64;

		*(bits++) = static_cast<uint8_t>((length64 >> 56) & LastByteMask);
		*(bits++) = static_cast<uint8_t>((length64 >> 48) & LastByteMask);
		*(bits++) = static_cast<uint8_t>((length64 >> 40) & LastByteMask);
		*(bits++) = static_cast<uint8_t>((length64 >> 32) & LastByteMask);
		*(bits++) = static_cast<uint8_t>((length64 >> 24) & LastByteMask);
		*(bits++) = static_cast<uint8_t>((length64 >> 16) & LastByteMask);
		*(bits++) = static_cast<uint8_t>((length64 >> 8) & LastByteMask);
		*(bits++) = static_cast<uint8_t>(length64 & LastByteMask);
	}

	uint32_t key = 0;

	if (masked)
	{
		key = overrideMask == 0 ? Randomizer<uint32_t>()() : overrideMask;

		*(bits++) = static_cast<uint8_t>((key & 0x000000FF));
		*(bits++) = static_cast<uint8_t>((key & 0x0000FF00) >> 8);
		*(bits++) = static_cast<uint8_t>((key & 0x00FF0000) >> 16);
		*(bits++) = static_cast<uint8_t>((key & 0xFF000000) >> 24);
	}

	const size_t headerSize = static_cast<size_t>(reinterpret_cast<std::byte*>(bits) - &headerBytes[0]);

	return std::tuple{headerSize, key};
}


std::string createSecWebSocketKeyValue()
{
	Randomizer<uint64_t> randomizer;

	uint64_t bytes[2] = {randomizer(), randomizer()};

	std::string key;

	CryptoPP::AlgorithmParameters params = CryptoPP::MakeParameters
		(CryptoPP::Name::Pad(), true)
		(CryptoPP::Name::InsertLineBreaks(), false);

	auto encoder = new CryptoPP::Base64Encoder(new CryptoPP::StringSink(key));

	encoder->IsolatedInitialize(params);

	CryptoPP::StringSource ss(reinterpret_cast<const byte*>(&bytes), sizeof(bytes), true, encoder);

	return key;
}


std::string createSecWebSocketAcceptValue(std::string_view secWebSocketKey)
{
	std::string acceptKey = format("%1%2", secWebSocketKey, HandshakeGuid);
	byte digest[CryptoPP::SHA::DIGESTSIZE];

	CryptoPP::SHA().CalculateDigest(digest, (const byte*)acceptKey.data(), acceptKey.size());

	std::string hash;

	CryptoPP::AlgorithmParameters params = CryptoPP::MakeParameters
		(CryptoPP::Name::Pad(), true)
		(CryptoPP::Name::InsertLineBreaks(), false);

	auto encoder = new CryptoPP::Base64Encoder(new CryptoPP::StringSink(hash));

	encoder->IsolatedInitialize(params);

	CryptoPP::StringSource ss(digest, sizeof(digest), true, encoder);

	return hash;
}


void applyMaskingKey(uint32_t maskingKey, std::byte* payloadPtr, size_t payloadSize)
{ // refactor:  always using quad word mask applying - just start from correctly aligned address (masking key just must be recomputed with corresponding bits offset.
	constexpr auto DwordSize = sizeof(uint32_t);
	constexpr auto QwordSize = sizeof(uint64_t);

	const std::byte bitMask[] =
	{
		static_cast<std::byte>(maskingKey & 0x000000FF),
		static_cast<std::byte>((maskingKey & 0x0000FF00) >> 8),
		static_cast<std::byte>((maskingKey & 0x00FF0000) >> 16),
		static_cast<std::byte>((maskingKey & 0xFF000000) >> 24)
	};


	size_t byteIndex = 0;

	if ((reinterpret_cast<ptrdiff_t>(payloadPtr) % alignof(uint64_t)) == 0 && (payloadSize >= QwordSize ) )
	{
		const uint64_t qwordMask = static_cast<uint64_t>(maskingKey) | (static_cast<uint64_t>(maskingKey) << 32);
		uint64_t* const qwordPtr = reinterpret_cast<uint64_t*>(payloadPtr);
		const size_t qwordCount = payloadSize / QwordSize;

		for (size_t i = 0; i < qwordCount; ++i)
		{
			qwordPtr[i] ^= qwordMask;
		}

		byteIndex = QwordSize * qwordCount;
	}
	else if ((reinterpret_cast<ptrdiff_t>(payloadPtr) % alignof(uint32_t)) == 0 && (payloadSize >= DwordSize ))
	{
		uint32_t* const dwordPtr = reinterpret_cast<uint32_t*>(payloadPtr);
		const size_t dwordCount = payloadSize / DwordSize;

		for (size_t i = 0; i < dwordCount; ++i)
		{
			dwordPtr[i] ^= maskingKey;
		}

		byteIndex = DwordSize * dwordCount;
	}
	else
	{
		const size_t dwordCount = payloadSize / DwordSize;

		for (size_t i = 0; i < dwordCount; ++i)
		{
			const size_t idx = i * 4;
		
			payloadPtr[idx + 0] ^= bitMask[0];
			payloadPtr[idx + 1] ^= bitMask[1];
			payloadPtr[idx + 2] ^= bitMask[2];
			payloadPtr[idx + 3] ^= bitMask[3];
		}

		byteIndex = DwordSize * dwordCount;
	}

	for (;byteIndex < payloadSize; ++byteIndex)
	{
		payloadPtr[byteIndex] ^= bitMask[byteIndex % 4];
	}
}

} // namespace cold::remoting::websocket
