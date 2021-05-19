#include "pch.h"
#include "websocketstream.h"
#include "transport.h"
#include "cold/com/comclass.h"
#include "cold/remoting/socketaddress.h"

using namespace cold::async;

namespace cold::network {

class WebSocketClient final : public TransportClient, public WebSocketControl
{
	COMCLASS_(TransportClient, WebSocketControl)

public:

	WebSocketClient(ComPtr<TransportClient> client)
		: m_client(client)
		, m_stream(WebSocketStream::create(client))
	{
	}

	~WebSocketClient()
	{
		// LOG_info_("~WebSocketClient()");
		this->dispose();
	}

private:

	void dispose() override
	{
		m_stream->dispose();
	}

	Task<BytesBuffer> read() override
	{
		return m_stream->read();
	}

	Task<> write(BufferView buffer) override
	{
		return m_stream->write(std::move(buffer));
	}

	Task<bool> connect(std::string_view address) override;

	size_t setInboundBufferSize(size_t size) override
	{
		return m_stream->setInboundBufferSize(size);
	}

	void setWebSocketMode(WebSocketMode mode) override
	{
		m_stream->setWebSocketMode(mode);
	}

	WebSocketMode webSocketMode() const override
	{
		return m_stream->webSocketMode();
	}

	void setMasked(bool value) override
	{
		m_stream->setMasked(value);
	}

	bool isMasked() const override
	{
		return m_stream->isMasked();
	}

	Stream::Ptr rawStream() const override
	{
		return m_stream->rawStream();
	}

	void setWriteStreamFrameSize(size_t streamFrameSize) override
	{
		m_stream->setWriteStreamFrameSize(streamFrameSize);
	}

	size_t writeStreamFrameSize() const override
	{
		return m_stream->writeStreamFrameSize();
	}

	const ComPtr<TransportClient> m_client;
	const ComPtr<WebSocketStream> m_stream;
};


Task<bool> WebSocketClient::connect(std::string_view address)
{
	const bool connected = co_await m_client->connect(address);

	if (connected)
	{
		co_await m_stream->clientToServerHandshake("");
	}

	co_return connected;
}


//-----------------------------------------------------------------------------
ComPtr<TransportClient> createWebSocketClient(ComPtr<TransportClient> client)
{
	DEBUG_CHECK(client)
	return com::createInstance<WebSocketClient, TransportClient>(std::move(client));
}


} // namespace cold::network
