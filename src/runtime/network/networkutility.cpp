#include "pch.h"
#include "networkutility.h"
#include "cold/runtime/internal/uvutils.h"

using namespace cold;
using namespace cold::async;


std::string makePipeFilePath(std::string_view host, std::string_view service)
{
	std::string pipeFilePath;

	pipeFilePath = host.empty() ?
		format("\\\\.\\pipe\\%1", service):
		format("\\\\%1\\pipe\\%2", host, service);

	return pipeFilePath;
}


Task<sockaddr> resolveSockAddr(uv_loop_t* loop, const char* host, const char* service, int protocol)
{
	auto resolveCallback = [](uv_getaddrinfo_t* request, int status, addrinfo* addr) noexcept
	{
		TaskSource<sockaddr>* const resolver = reinterpret_cast<TaskSource<sockaddr>*>(request->data);

		if (status == 0)
		{
			DEBUG_CHECK(addr)
			resolver->resolve(*addr->ai_addr);
		}
		else
		{
			const auto message = uvErrorMessage(status);
			resolver->reject(Excpt_(message));
		}

		if (addr)
		{
			uv_freeaddrinfo(addr);
		}
	};

	addrinfo hints;
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = protocol;
	hints.ai_flags = 0;

	uv_getaddrinfo_t request;
	TaskSource<sockaddr> resolver;
	request.data = &resolver;

	const char* LocalHost = "0.0.0.0";
	const char* actualHost = (strlen(host) == 0 || strcmp(host, ".") == 0) ? LocalHost : host;

	UV_RUNTIME_CHECK(uv_getaddrinfo(loop, &request, resolveCallback, actualHost, service, &hints))

	const auto addr = co_await resolver.getTask();

	co_return addr;
}