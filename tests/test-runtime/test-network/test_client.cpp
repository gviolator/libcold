#include "pch.h"
#include "testnetworkbase.h"
#include "stopwatch.h"
#include <cold/threading/event.h>

using namespace cold;
using namespace cold::async;
using namespace cold::network;
using namespace testing;

using namespace std::chrono_literals;

namespace {

using Test_Network_Client  = Test_Network_Default;

//-----------------------------------------------------------------------------
TEST_P(Test_Network_Client, ConnectBasic)
{
	auto serverTask = [&]() -> Task<bool>
	{
		auto server = co_await Server::listen(this->address());
		[[maybe_unused]] auto stream = co_await server->accept();
		co_return static_cast<bool>(stream);
	}();

	auto connectTask = Client::connect(this->address());

	EXPECT_NO_THROW(async::waitResult(std::ref(connectTask)));
	EXPECT_NO_THROW(async::waitResult(std::ref(serverTask)));
	ASSERT_TRUE(serverTask.result());
}


TEST_P(Test_Network_Client, ConnectionTimeout)
{
	using namespace std::chrono;

	constexpr milliseconds Timeout = 75ms;

	const StopWatch timer;

	auto task = Client::connect(this->address(), Timeout);

	EXPECT_THROW(async::waitResult(std::ref(task)), network::NetworkException);
	EXPECT_THAT(timer.ms(), Ge(Timeout.count()));
}


TEST_P(Test_Network_Client, ConnectionCancellation)
{
	using namespace std::chrono;

	constexpr milliseconds Timeout = 75ms;

	//using_(client) = Client::create();

	auto cancellation = CancellationTokenSource::create();

	[&]() -> Task<>
	{
		co_await (Timeout + 5ms);
		cancellation->cancel();
	}().detach();

	const StopWatch timer;
	auto task = Client::connect(this->address(), cancellation->token());

	EXPECT_THROW(async::waitResult(std::ref(task)), network::NetworkException);
	EXPECT_THAT(timer.ms(), Ge(Timeout.count()));
}


TEST_P(Test_Network_Client, BreakConnectOnRuntimeShutdown)
{
	//auto task = [&]() -> Task<>
	//{
	//	[[maybe_unused]] Stream::Ptr client = co_await Client::connect(this->address());
	//}();

	auto task = Client::connect(this->address());

	std::this_thread::sleep_for(30ms);

	resetRuntime();
	
	EXPECT_THROW(async::waitResult(std::ref(task)), std::exception);
}

}


INSTANTIATE_TEST_CASE_P(
	Default,
	Test_Network_Client,
	testing::ValuesIn(Test_Network_Base::addresses())
);
