#pragma once
#include "pch.h"
#include "runtimescopeguard.h"
#include <cold/runtime/runtime.h>
#include <cold/network/server.h>
#include <cold/network/client.h>
#include <cold/network/networkexception.h>
#include <cold/memory/rtstack.h>

class Test_Network_Base
{
public:

	static std::vector<std::string> addresses();

	static cold::async::Task<cold::BytesBuffer> readEos(cold::network::Stream::Ptr stream);

	static cold::async::Task<cold::BytesBuffer> readCount(cold::network::Stream::Ptr stream, size_t);

protected:

	virtual std::string address() const = 0;

	void resetRuntime();

private:

	RuntimeScopeGuard m_runtime;
	rtstack(cold::Megabyte(1));
};



class Test_Network_Default : public Test_Network_Base, public testing::TestWithParam<std::string>
{
protected:

	std::string address() const override;
};

