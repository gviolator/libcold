#include "pch.h"
#include "websocketstream.h"
#include "transport.h"
#include "cold/com/comclass.h"

using namespace cold::async;


namespace cold::network {

class WebSocketServer final : public TransportServer
{
	COMCLASS_(TransportServer)

public:

	WebSocketServer(ComPtr<TransportServer> server): m_server(std::move(server))
	{}


private:

	void dispose() override
	{
		m_isDisposed = true;
		m_server->dispose();
	}

	Task<> listen(std::string_view address, std::optional<unsigned> backlog) override
	{
		return m_server->listen(address, backlog);
	}

	Task<Stream::Ptr> accept() override
	{
		while (!this->m_isDisposed)
		{
			Stream::Ptr inboundStream = co_await m_server->accept(); 
			if (!inboundStream)
			{
				break;
			}

			ComPtr<WebSocketStream> wsStream = WebSocketStream::create(std::move(inboundStream));

			try
			{
				co_await wsStream->serverToClientHandshake();
				wsStream->setMasked(false);
				co_return std::move(wsStream);
			}
			catch (const std::exception& exception)
			{
				LOG_warning_("Accepted web client, but handshake is failed:[%1]", exception.what())
			}
		}

		co_return Stream::Ptr{};
	}


	const ComPtr<TransportServer> m_server;
	bool m_isDisposed = false;
};


//-----------------------------------------------------------------------------
ComPtr<TransportServer> createWebSocketServer(ComPtr<TransportServer> server)
{
	DEBUG_CHECK(server)
	return com::createInstance<WebSocketServer, TransportServer>(std::move(server));
}

} // namespace cold::network
