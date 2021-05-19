#pragma once
#include "uvstream.h"
#include "transport.h"

#include "cold/network/server.h"
#include "cold/runtime/disposableruntimeguard.h"
#include "cold/threading/critical_section.h"


namespace cold::network {

class ABSTRACT_TYPE UvServer : public TransportServer
{
	DECLARE_CLASS_BASE(TransportServer)

public:

	~UvServer();

	void dispose() override;

	async::Task<Stream::Ptr> accept() override;

	async::Task<> listen(std::string_view, std::optional<unsigned> backlog) override;

protected:

	virtual async::Task<UvStreamHandle> bind(std::string_view) = 0;

	virtual UvStreamHandle createClientHandle() = 0;

private:

	void resolveAccept(Stream::Ptr);

	void doDispose(bool destructed) noexcept;

	UvStreamHandle m_server;
	async::TaskSource<Stream::Ptr> m_acceptAwaiter = nothing;
	std::list<Stream::Ptr> m_inboundConnections;
	threading::CriticalSection m_mutex;
	std::optional<DisposableRuntimeGuard> m_disposableGuard;
	std::atomic_bool m_isDisposed = false;
};

}
