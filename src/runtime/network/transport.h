#pragma once
#include "cold/com/ianything.h"
#include "cold/network/client.h"
#include "cold/network/server.h"
#include "cold/utils/intrusivelist.h"
#include "cold/runtime/internal/uvutils.h"

#include <string_view>

namespace cold::network {


struct ABSTRACT_TYPE TransportClient : virtual Stream
{
	DECLARE_CLASS_BASE(Stream)

	virtual async::Task<bool> connect(std::string_view address) = 0;
};


struct ABSTRACT_TYPE TransportServer : Server
{
	DECLARE_CLASS_BASE(Server)

	virtual async::Task<> listen(std::string_view address, std::optional<unsigned> backlog) = 0;
};


struct TransportFactory
{
	static ComPtr<TransportClient> createClient(std::string_view protocol) noexcept;

	static ComPtr<TransportServer> createServer(std::string_view protocol) noexcept;
};


struct TransportFactoryRegistry : IntrusiveListElement<TransportFactoryRegistry>
{
	using ClientFactory = ComPtr<TransportClient> (*) (std::string_view);
	using ServerFactory = ComPtr<TransportServer> (*) (std::string_view);


	const ClientFactory clientFactory = nullptr;
	const ServerFactory serverFactory = nullptr;

	TransportFactoryRegistry(ClientFactory, ServerFactory);
};


}
