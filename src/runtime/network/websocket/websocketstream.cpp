#include "pch.h"
#include "websocketstream.h"
#include "cold/network/networkexception.h"

#include "cold/remoting/httpparser.h"
#include "cold/remoting/websocket.h"
#include "cold/memory/rtstack.h"
#include "cold/com/comclass.h"

using namespace cold::async;

namespace cold::network {

using StackString = std::basic_string<char, std::char_traits<char>, RtStackStdAllocator<char>>;

/**
* 
*/
class WebSocketStreamImpl final : public WebSocketStream
{
	COMCLASS_(WebSocketStream)

public:

	WebSocketStreamImpl(Stream::Ptr stream): m_stream(std::move(stream))
	{}

	~WebSocketStreamImpl()
	{
		this->dispose();
	}

	void dispose() override;

	Task<BytesBuffer> read() override;

	Task<> write(BufferView) override;

	size_t setInboundBufferSize(size_t size) override;

	Task<> clientToServerHandshake(std::string_view host) override;

	Task<> serverToClientHandshake() override;

	void setWebSocketMode(WebSocketMode mode) override
	{
		m_mode = mode;
	}

	WebSocketMode webSocketMode() const override
	{
		return m_mode;
	}

	void setMasked(bool value) override
	{
		m_isMasked = value;
	}

	bool isMasked() const override
	{
		return m_isMasked;
	}

	Stream::Ptr rawStream() const override
	{
		return m_stream;
	}

	void setWriteStreamFrameSize(size_t streamFrameSize) override
	{
		m_writeStreamFrameSize = streamFrameSize;
	}

	size_t writeStreamFrameSize() const override
	{
		return m_writeStreamFrameSize;
	}

private:

	Task<HttpParser> readHttpPrefix()
	{
		DEBUG_CHECK(m_stream)

		do
		{
			if (const HttpParser http(asStringView(m_messageReader.buffer())); http)
			{ // http refer to the m_messageReader.buffer() content, so calling side should pop bytes from reader.
				co_return http;
			}

			auto buffer = co_await m_stream->read();

			if (!buffer)
			{
				throw Excpt(NetworkException, "Remote point was closed prior web-socket handshake was completed");
			}

			m_messageReader.addBuffer(std::move(buffer));
		}
		while (true);
	}


	Stream::Ptr m_stream;
	websocket::MessageReader m_messageReader;
	WebSocketMode m_mode = WebSocketMode::Binary;
	size_t m_writeStreamFrameSize = 0;
	bool m_isMasked = true;
};

//-----------------------------------------------------------------------------
void WebSocketStreamImpl::dispose()
{
	if (!m_stream)
	{
		return;
	}

	[](Stream::Ptr stream, bool masked) -> Task<>
	{
		SCOPE_Leave
		{
			stream->dispose();
		};

		websocket::FrameHeaderBytes header;
		auto [headerSize, maskingKey] = websocket::initializeFrameHeaderBytes(header, websocket::FrameType::CloseConnection, true, masked, 0);
		BytesBuffer frame(headerSize);
		memcpy(frame.data(), header, headerSize);

		try
		{
			co_await stream->write(frame.toReadOnly());
		}
		catch (const std::exception&)
		{
			LOG_debug_("no write")
		}
	}
	(std::move(m_stream), m_isMasked).detach();
}


Task<BytesBuffer> WebSocketStreamImpl::read()
{
	using FrameType = websocket::FrameType;

	KeepRC(*this);

	if (!m_stream)
	{
		co_return BytesBuffer{};
	}

	while (true)
	{
		if (auto frame = m_messageReader.getMessageFrame(); frame)
		{
			switch (frame.type)
			{
			case FrameType::Binary:
			case FrameType::Utf8Text:
			{
				co_return std::move(frame.payload);
			}

			case FrameType::Ping:
			{
				LOG_debug_("Web socket: PING");
				break;
			}

			case FrameType::CloseConnection:
			{
				if (m_stream)
				{
					m_stream->dispose();
					m_stream.reset();
				}

				co_return BytesBuffer{};
			}

			default:
			{
				LOG_warning_("Web socket unhandled frame type");
			}
			}
		}

		if (auto buffer = m_stream ? co_await m_stream->read() : BytesBuffer {}; buffer)
		{
			m_messageReader.addBuffer(std::move(buffer));
		}
		else
		{
			break;
		}
	}

	co_return BytesBuffer{};
}


Task<> WebSocketStreamImpl::write(BufferView payload)
{
	using namespace cold::websocket;

	if (!m_stream)
	{
		throw Excpt_("Underlying transport stream is closed.");
	}

	const websocket::FrameType wholeMessageType = (m_mode == WebSocketMode::Binary) ? FrameType::Binary : FrameType::Utf8Text;

	const size_t streamedFramePayloadSize = m_writeStreamFrameSize > 0 ? m_writeStreamFrameSize : payload.size();
	uint32_t messageMask = 0;
	size_t payloadOffset = 0;
	

	do
	{ // event if payload is empty websocket message must still be send.
		const size_t payloadSize = std::min(streamedFramePayloadSize, payload.size() - payloadOffset);
		
		const websocket::FrameType frameType = payloadOffset == 0 ? wholeMessageType : websocket::FrameType::ContinuePayload;
		const std::byte* const payloadPtr = payload.data() + payloadOffset;

		payloadOffset += payloadSize;

		websocket::FrameHeaderBytes header;
		auto [headerSize, maskingKey] = websocket::initializeFrameHeaderBytes(header, frameType, m_isMasked, payloadSize, payloadOffset == payload.size(), messageMask);

		DEBUG_CHECK(messageMask == 0 || messageMask == maskingKey)
		DEBUG_CHECK(m_isMasked == (maskingKey != 0))

		messageMask = maskingKey;

		BytesBuffer frame(headerSize + payloadSize);
		memcpy(frame.data(), header, headerSize);

		if (payloadSize > 0)
		{
			memcpy(frame.data() + headerSize, payloadPtr, payloadSize);
			if (maskingKey != 0)
			{
				websocket::applyMaskingKey(maskingKey, frame.data() + headerSize, payloadSize);
			}
		}

		co_await m_stream->write(frame.toReadOnly());
	}
	while (payloadOffset < payload.size());
}


Task<> WebSocketStreamImpl::clientToServerHandshake(std::string_view host)
{
	BytesBuffer handshakeRequest;

	const auto clientKey = websocket::createSecWebSocketKeyValue();

	{
		rtstack();

		StackString handshake =
			"GET /services HTTP/1.1\r\n"
			"Sec-WebSocket-Version: 13\r\n"
			"Connection : Upgrade\r\n"
			"Upgrade: websocket\r\n"
			"Host: ";


		handshake = handshake + (host.empty() ? StackString("localhost") : StackString(host));
		handshake = handshake + "\r\nSec-WebSocket-Key: " + StackString{clientKey.data(), clientKey.size()} + StackString{HttpParser::EndOfHeaders};

		memcpy(handshakeRequest.append(handshake.size()), handshake.data(), handshake.size());
	}

	co_await m_stream->write(handshakeRequest.toReadOnly());

	const auto http = co_await readHttpPrefix();
	SCOPE_Leave
	{
		m_messageReader.popBytes(http.headersLength());
	};


	auto acceptKey = http["Sec-WebSocket-Accept"];
	auto expectedKey = websocket::createSecWebSocketAcceptValue(clientKey);
	if (acceptKey != expectedKey)
	{
		LOG_warning_("Invalid Sec-WebSocket-Accept: [%1], expected: [%2], Sec-WebSocket-Key: [%3]", acceptKey, expectedKey, clientKey)

		throw Excpt(NetworkException, "Invalid Sec-WebSocket-Accept value");
	}
}


Task<> WebSocketStreamImpl::serverToClientHandshake()
{
	DEBUG_CHECK(m_stream)

	BytesBuffer response;

	{
		const auto http = co_await readHttpPrefix();
		SCOPE_Leave
		{
			m_messageReader.popBytes(http.headersLength());
		};

		std::string_view clientKey = http["Sec-WebSocket-Key"];
		if (clientKey.empty())
		{
			throw Excpt_("[Sec-WebSocket-Key] header is missing");
		}

		rtstack();

		const auto webSocketAccept = websocket::createSecWebSocketAcceptValue(clientKey);

		StackString handshake =
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: ";

		handshake = handshake + StackString{webSocketAccept.data(), webSocketAccept.size()} +StackString{HttpParser::EndOfHeaders};
		memcpy(response.append(handshake.size()), handshake.data(), handshake.size());
	}

	co_await m_stream->write(response.toReadOnly());
}


size_t WebSocketStreamImpl::setInboundBufferSize(size_t size)
{
	if (SocketControl* const control = m_stream ? m_stream->as<SocketControl*>() : nullptr; control)
	{
		return control->setInboundBufferSize(size);
	}

	return 0;
}

//-----------------------------------------------------------------------------

ComPtr<WebSocketStream> WebSocketStream::create(Stream::Ptr stream)
{
	return com::createInstance<WebSocketStreamImpl, WebSocketStream>(std::move(stream));
}

}
