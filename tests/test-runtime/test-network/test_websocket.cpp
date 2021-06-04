#include "pch.h"
#include "buffertestutils.h"
#include <cold/remoting/websocket.h>


using namespace cold;
using namespace cold::strings;

using namespace testing;



namespace {

/// <summary>
///
/// </summary>
struct WsFrameTestData
{
	const size_t palyloadLength;
	const bool masked = false;
	const size_t readBlockSize = 0;


	WsFrameTestData(size_t palyloadLength_, bool masked_) : palyloadLength(palyloadLength_), masked(masked_)
	{}

	WsFrameTestData(size_t palyloadLength_, bool masked_, size_t readBlockSize_)
		: palyloadLength(palyloadLength_)
		, masked(masked_)
		, readBlockSize(readBlockSize_)
	{}
};


constexpr size_t Require7bitLength = 121;
constexpr size_t Require16bitLength = std::numeric_limits<uint16_t>::max() / 2;
constexpr size_t Require64bitLength = std::numeric_limits<uint16_t>::max() * 2;
constexpr bool UseMaskingKey = true;
constexpr bool NoMaskingKey = false;

}


/// <summary>
///
/// </summary>
class Network_WebSocketParametrized : public testing::TestWithParam<WsFrameTestData>
{
protected:

	size_t expectedPalyloadLength() const
	{
		return GetParam().palyloadLength;
	}

	bool masked() const
	{
		return GetParam().masked;
	}

};


/// <summary>
///
/// </summary>
class Network_WebSocketHeader : public Network_WebSocketParametrized
{};


/// <summary>
///
/// </summary>
class Network_WebSocketReader : public Network_WebSocketParametrized
{
protected:

	struct Frame
	{
		ReadOnlyBuffer originalPayload;
		ReadOnlyBuffer payload;
		uint32_t mask;
		bool finalized;
	};


	Frame makeWsFrame(bool finalized = true) const
	{
		using namespace cold::websocket;

		constexpr auto What = FrameType::Binary;

		const size_t payloadLength = GetParam().palyloadLength;
		auto originalPayload = createBufferWithDefaultContent(payloadLength);

		FrameHeaderBytes headerBytes;
		auto[headerSize, mask] = initializeFrameHeaderBytes(headerBytes, What, masked(), payloadLength, finalized);


		BytesBuffer frame(headerSize + payloadLength);
		memcpy(frame.data(), headerBytes, headerSize);
		memcpy(frame.data() + headerSize, originalPayload.data(), originalPayload.size());

		if (mask != 0)
		{
			applyMaskingKey(mask, frame.data() + headerSize, payloadLength);
		}

		return {originalPayload.toReadOnly(), frame.toReadOnly(), mask, finalized };
	}
};


class Network_WebSocketMultiFrame: public Network_WebSocketParametrized
{};

//-----------------------------------------------------------------------------
TEST_P(Network_WebSocketHeader, InitializeAndParseFrameHeader)
{
	using namespace cold::websocket;

	constexpr FrameType ExpectedFrameType = FrameType::Utf8Text;
	constexpr std::array finFlags = {true, false};

	for (const bool finalized : finFlags)
	{
		FrameHeaderBytes bytes;

		const uint64_t payloadLength = this->expectedPalyloadLength();

		auto [headerSize, originalMaskingKey] = initializeFrameHeaderBytes(bytes, ExpectedFrameType, this->masked(), payloadLength, finalized );

		auto header = parseFrameHeader(bytes, headerSize);

		ASSERT_TRUE(header);
	
		ASSERT_THAT(header->headerSize, Eq(headerSize));
		ASSERT_THAT(header->payloadLength, Eq(payloadLength));
		ASSERT_THAT(header->type, Eq(ExpectedFrameType));
		ASSERT_THAT(header->isFinal, Eq(finalized));

		if (masked())
		{
			ASSERT_THAT(header->maskingKey, Not(Eq(0)));
			ASSERT_THAT(header->maskingKey, Eq(originalMaskingKey));
		}
		else
		{
			ASSERT_THAT(header->maskingKey, Eq(0));
		}
	}
}


TEST_P(Network_WebSocketReader, ReadMessage)
{
	websocket::MessageReader reader;

	auto [originalBuffer, messageBuffer, mask, _] = makeWsFrame();

	const auto readBlockSize = std::min(messageBuffer.size(), GetParam().readBlockSize == 0 ? messageBuffer.size() : GetParam().readBlockSize);

	DEBUG_CHECK(readBlockSize > 0)

	while (reader.buffer().size() < messageBuffer.size())
	{
		ASSERT_FALSE(reader.getMessageFrame());

		const size_t readOffset = reader.buffer().size();
		const size_t actualBlockSize = std::min(readBlockSize, messageBuffer.size() - readOffset);

		reader.addBuffer(BufferView{messageBuffer, readOffset, actualBlockSize});
	}

	auto message = reader.getMessageFrame();

	ASSERT_TRUE(message);

	ASSERT_THAT(message.payload.size(), Eq(expectedPalyloadLength()));
	ASSERT_TRUE(buffersEqual(BufferView{originalBuffer}, message.payload.toReadOnly()));
}


TEST_P(Network_WebSocketMultiFrame, NoStream)
{
	using namespace cold::websocket;


	constexpr size_t FramesCount = 5;
	constexpr FrameType ExpectedFrameType = FrameType::Binary;

	websocket::MessageReader reader;

	auto messagePayload = createBufferWithDefaultContent(expectedPalyloadLength()).toReadOnly();

	
	const size_t FramePayloadSize = messagePayload.size() / FramesCount;

	size_t readOffset = 0;


	uint32_t maskKey = 0;

	while (readOffset < messagePayload.size())
	{
		ASSERT_FALSE(reader.getMessageFrame());

		const size_t payloadSize = std::min(FramePayloadSize, messagePayload.size() - readOffset);
		const FrameType frameType = readOffset == 0 ? ExpectedFrameType : FrameType::ContinuePayload;

		const std::byte* readPtr = messagePayload.data() + readOffset;

		readOffset += payloadSize;

		FrameHeaderBytes headerBytes;

		auto[headerSize, mask] = initializeFrameHeaderBytes(headerBytes, frameType, this->masked(), payloadSize, readOffset == messagePayload.size(), maskKey);
		ASSERT_TRUE(maskKey == 0 || maskKey == mask);
		ASSERT_TRUE(this->masked() == (mask != 0));

		maskKey = mask;
		
		BytesBuffer frame(payloadSize + headerSize);
		memcpy(frame.data(), headerBytes, headerSize);
		memcpy(frame.data() + headerSize, readPtr, payloadSize);
		applyMaskingKey(mask, frame.data() + headerSize, payloadSize);

		reader.addBuffer(std::move(frame));
	}

	auto message = reader.getMessageFrame();
	ASSERT_TRUE(message);
	ASSERT_THAT(message.type, Eq(ExpectedFrameType));
	ASSERT_TRUE(buffersEqual(BufferView{messagePayload}, message.payload.toReadOnly()));
};

//-----------------------------------------------------------------------------

#if 0

class Network_WebSocket
	: public NetworkTestFixtureBase
	, public testing::Test
{

};
#endif

#if 0
TEST_F(Network_WebSocket, ClientConnect)
{
	WorkerEntry worker;

	auto clientFactory = [](IoStreamPtr client) -> Task<>
	{
		LOG_Info("Start reading client socket...")

		while (true)
		{
			auto buffer = co_await client->read();

			if (!buffer)
			{
				break;
			}
		}

		LOG_Info("Done")
	};



	auto task = run([this, clientFactory]() -> Task<>
	{
		auto client = INetworkWorker::current()->createClient(Transport::WebSocket);

		co_await client->connect({}, L"7789");

		//while (true)
		{
			auto message = co_await client->read();

			if (!message)
			{
				//break;
			}


			std::string text(asStringView(message));

			{
				std::string response = text + "_" + text;

				BytesBuffer buffer;

				buffer.write(response.data(), response.length());

				co_await client->write(buffer.toReadOnly());
			}

		}


	}, worker.scheduler());


	EXPECT_AWAITABLE_RESULT(task);

}
#endif

//-----------------------------------------------------------------------------

INSTANTIATE_TEST_CASE_P(
	Default,
	Network_WebSocketHeader,
	testing::Values(
		WsFrameTestData{ Require7bitLength, NoMaskingKey },
		WsFrameTestData{ Require7bitLength, UseMaskingKey },
		WsFrameTestData{ Require16bitLength, NoMaskingKey },
		WsFrameTestData{ Require16bitLength, UseMaskingKey },
		WsFrameTestData{ Require64bitLength, NoMaskingKey },
		WsFrameTestData{ Require64bitLength, UseMaskingKey }
	));

INSTANTIATE_TEST_CASE_P(
	Default,
	Network_WebSocketReader,
	testing::Values(
		WsFrameTestData{ 20, UseMaskingKey, 5},
		WsFrameTestData{ 300, NoMaskingKey, 100 },
		WsFrameTestData{ 300, UseMaskingKey, 17 },
		WsFrameTestData{ 250, UseMaskingKey},
		WsFrameTestData{ Require16bitLength, UseMaskingKey, Require16bitLength /20 },
		WsFrameTestData{ Require64bitLength, UseMaskingKey, Require64bitLength /20 }
));


INSTANTIATE_TEST_CASE_P(
	Default,
	Network_WebSocketMultiFrame,
	testing::Values(
		WsFrameTestData{ 10, UseMaskingKey},
		WsFrameTestData{ 10, NoMaskingKey},
		WsFrameTestData{ 128 * 5, UseMaskingKey},
		WsFrameTestData{ Require16bitLength, UseMaskingKey },
		WsFrameTestData{ Require64bitLength, UseMaskingKey }
));
