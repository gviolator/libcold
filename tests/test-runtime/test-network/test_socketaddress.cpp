#include "pch.h"
#include <cold/remoting/socketaddress.h>

using namespace cold;
using namespace cold::remoting;
using namespace testing;


TEST(Test_SocketAddress, ParseAddressString)
{
	ASSERT_NO_THROW(SocketAddress::parseAddressString("tcp://192.168.1.2:6060"));
	ASSERT_NO_THROW(SocketAddress::parseAddressString("ipc://path"));
	ASSERT_NO_THROW(SocketAddress::parseAddressString("any_protocol://value"));
	ASSERT_NO_THROW(SocketAddress::parseAddressString("ws+tcp://localhost:70"));
	ASSERT_THROW(SocketAddress::parseAddressString("ws+tcp:/localhost:70"), std::exception);
	ASSERT_THROW(SocketAddress::parseAddressString("ws!tcp://localhost:70"), std::exception);
}

TEST(Test_SocketAddress, ParseAddressString2)
{
	auto [protocol, address] = SocketAddress::parseAddressString("ws+tcp://localhost:70");
	EXPECT_THAT(protocol, Eq("ws+tcp"));
	EXPECT_THAT(address, Eq("localhost:70"));
}

TEST(Test_SocketAddress, ParseTcp)
{
	const auto verify = [](std::string_view host, std::string_view service)
	{
		const auto address = strfmt("{0}:{1}", host, service);
		auto [resHost, resService] = SocketAddress::parseTcpAddress(address);

		EXPECT_THAT(resHost, Eq(host));
		EXPECT_THAT(resService, Eq(service));
	};

	EXPECT_NO_FATAL_FAILURE(verify("192.168.1.2", "6060"));
	EXPECT_NO_FATAL_FAILURE(verify("localhost", "6060"));
	EXPECT_NO_FATAL_FAILURE(verify("", "80"));
	ASSERT_THROW(verify("192.168.1.2", "ab"), std::exception);
	ASSERT_THROW(verify("192.$$.1.2", "80"), std::exception);
}

TEST(Test_SocketAddress, ParseIpc)
{
	const auto verify = [](std::string_view host, std::string_view service)
	{
		const auto address = strfmt("{0}\\{1}", host, service);
		auto [resHost, resService] = SocketAddress::parseIpcAddress(address);

		if (host != ".")
		{
			EXPECT_THAT(resHost, Eq(host));
		}
		else
		{
			EXPECT_THAT(resHost, Eq(""));
		}

		EXPECT_THAT(resService, Eq(service));
	};

	EXPECT_NO_FATAL_FAILURE(verify(".", "path1"));
	EXPECT_NO_FATAL_FAILURE(verify("localhost", "path1"));
	EXPECT_NO_FATAL_FAILURE(verify("localhost", "pipename.txt"));

	ASSERT_THROW(SocketAddress::parseIpcAddress(".\\slash\\slash"), std::exception);
	ASSERT_THROW(SocketAddress::parseIpcAddress("no_host_or_path"), std::exception);
}
