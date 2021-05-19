#include "pch.h"
#include "transport.h"



namespace cold::network {


ComPtr<TransportClient> createWebSocketClient(ComPtr<TransportClient> underlyingClient);
ComPtr<TransportServer> createWebSocketServer(ComPtr<TransportServer> underlyingServer);


namespace {


std::string_view tryGetWebSocketUnderlyingProtocol(std::string_view protocol)
{
	auto iter = strings::split(protocol, "+").begin();
	if (*iter != "ws")
	{
		return {};
	}

	return (*++iter).empty() ? std::string_view{"tcp"} : *iter;
}

const TransportFactoryRegistry transportRegistry(

	[](std::string_view protocol) -> ComPtr<TransportClient>
	{
		std::string_view underlyingProtocol = tryGetWebSocketUnderlyingProtocol(protocol);
		if (underlyingProtocol.empty())
		{
			return {};
		}

		auto underlyingClient = TransportFactory::createClient(underlyingProtocol);
		return underlyingClient ? createWebSocketClient(std::move(underlyingClient)) : ComPtr<TransportClient>{};
	},

	[](std::string_view protocol) -> ComPtr<TransportServer>
	{
		std::string_view underlyingProtocol = tryGetWebSocketUnderlyingProtocol(protocol);
		if (underlyingProtocol.empty())
		{
			return {};
		}

		auto underlyingServer = TransportFactory::createServer(underlyingProtocol);
		return underlyingServer ? createWebSocketServer(std::move(underlyingServer)) : ComPtr<TransportServer>{};
	}
);

} // namespace
} // namespace cold::network
