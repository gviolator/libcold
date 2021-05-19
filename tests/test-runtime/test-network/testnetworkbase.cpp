#include "pch.h"
#include "testnetworkbase.h"
#include <cold/network/utils/tcptablesnapshot.h>
#include <cold/memory/rtstack.h>


using namespace cold;
using namespace cold::async;

//-----------------------------------------------------------------------------
void Test_Network_Base::resetRuntime()
{
	m_runtime.reset();
}


std::vector<std::string> Test_Network_Base::addresses()
{
	const auto tcpPortBusy = [](int port, const network::TcpEntryHandle* handles, size_t count) -> bool
	{
		for (size_t i = 0; i < count; ++i)
		{
			if (const network::TcpTableEntry entry {handles[i]}; entry.localPort == port)
			{
				return true;
			}
		}
		return false;
	};

	const auto chooseFreeTcpPort = [tcpPortBusy](unsigned portStart) -> unsigned
	{
		return network::getTcpTableSnapshot([&tcpPortBusy, portStart](const network::TcpEntryHandle* handles, size_t count) -> unsigned
		{
			unsigned port = portStart;

			while (tcpPortBusy(port, handles, count))
			{
				++port;
			}
			
			return port;
		});
	};

	const unsigned port = chooseFreeTcpPort(5770);
	constexpr std::string_view PipeChannelName = ".\\libcold-test-channel";

	return {
		format("tcp://:{0}", port),
		format("ipc://{0}", PipeChannelName),
		format("ws+tcp://:{0}", port),
		format("ws+ipc://{0}", PipeChannelName)
	};
}


Task<BytesBuffer> Test_Network_Base::readEos(cold::network::Stream::Ptr stream)
{
	BytesBuffer result;

	do
	{
		auto inboundBuffer = co_await stream->read();
		if (inboundBuffer.size() == 0)
		{
			break;
		}

		result += std::move(inboundBuffer);
	}
	while(true);

	co_return result;
}


Task<BytesBuffer> Test_Network_Base::readCount(network::Stream::Ptr stream, size_t bytesCount)
{
	BytesBuffer buffer;

	while (buffer.size() < bytesCount)
	{
		BytesBuffer frame = co_await stream->read();
		buffer += std::move(frame);
	}

	co_return buffer;
}

//-----------------------------------------------------------------------------
std::string Test_Network_Default::address() const
{
	return GetParam();
}
