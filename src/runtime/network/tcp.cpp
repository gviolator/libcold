#include "pch.h"
#include "uvserver.h"
#include "networkutility.h"
#include "cold/remoting/socketaddress.h"
#include "cold/runtime/internal/runtimeinternal.h"
#include "cold/diagnostics/exceptionguard.h"
#include "cold/com/comclass.h"

using namespace cold::async;


namespace cold::network {

/**
* 
*/
class TcpServer final : public UvServer
{
	COMCLASS_(UvServer)


private:

	Task<UvStreamHandle> bind(std::string_view address) override
	{
		auto [host, service] = remoting::SocketAddress::parseTcpAddress(address);

		const sockaddr sockAddr = co_await resolveSockAddr(RuntimeInternal::uv(), host.c_str(), service.c_str(), IPPROTO_TCP);

		UvHandle<uv_tcp_t> tcp;
		UV_RUNTIME_CHECK(uv_tcp_init(RuntimeInternal::uv(), tcp))
		UV_THROW_ON_ERROR(uv_tcp_bind(tcp, &sockAddr, 0))

		co_return tcp;
	}

	UvStreamHandle createClientHandle() override
	{
		UvHandle<uv_tcp_t> tcp;
		UV_RUNTIME_CHECK(uv_tcp_init(RuntimeInternal::uv(), tcp))

		return tcp;
	}
};


/**
* 
*/
class TcpClient final : public virtual UvStream, public virtual TransportClient
{
	COMCLASS_(UvStream, TransportClient)

public:
	
	TcpClient(): UvStream(createTcp())
	{}


private:

	Task<bool> connect(std::string_view address) override
	{
		const auto callback = [](uv_connect_t* connect, int status) noexcept
		{
			DEBUG_CHECK(connect)

			DEBUG_NOEXCEPT_Guard
			{
				auto& connectAwaiter = *reinterpret_cast<TaskSource<bool>*>(connect->data);

				if (status == 0)
				{
					connectAwaiter.resolve(true);
				}
				else if (status == UV_ECONNREFUSED)
				{
					connectAwaiter.resolve(false);
				}
				else
				{
					auto message = uvErrorMessage(status);
					connectAwaiter.reject(Excpt(NetworkException, message));
				}
			};
		};

		THROW_IF_DISPOSED(*this)

		if (!m_sockAddr)
		{
			auto [host, service] = remoting::SocketAddress::parseTcpAddress(address);
			m_sockAddr = co_await resolveSockAddr(RuntimeInternal::uv(), host.c_str(), service.c_str(), IPPROTO_TCP);
			THROW_IF_DISPOSED(*this)
		}

		TaskSource<bool> connectAwaiter;
		uv_connect_t connectRequest;
		connectRequest.data = &connectAwaiter;

		UV_RUNTIME_CHECK(uv_tcp_connect(&connectRequest, this->stream().as<uv_tcp_t>(), &m_sockAddr.value(), callback))

		const bool connected = co_await connectAwaiter.getTask();
		co_return connected;
	}

	static inline UvStreamHandle createTcp()
	{
		UvHandle<uv_tcp_t> tcp;
		UV_RUNTIME_CHECK(uv_tcp_init(RuntimeInternal::uv(), tcp))
		return tcp;
	}

	std::optional<sockaddr> m_sockAddr;
};


namespace {

const TransportFactoryRegistry transportRegistry(

	[](std::string_view protocol)
	{
		if (!strings::icaseEqual(protocol, "tcp"))
		{
			return ComPtr<TransportClient>{};
		}

		return com::createInstance<TcpClient, TransportClient>();
	},

	[](std::string_view protocol)
	{
		if (!strings::icaseEqual(protocol, "tcp"))
		{
			return ComPtr<TransportServer>{};
		}

		return com::createInstance<TcpServer, TransportServer>();
	});

} // namespace
} // namespace cold::network
