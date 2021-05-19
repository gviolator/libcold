#include "pch.h"
#include "testnetworkbase.h"
#include <cold/threading/event.h>
#include <cold/threading/barrier.h>
#include <cold/runtime/internal/runtimeinternal.h>


using namespace cold;
using namespace cold::async;
using namespace cold::network;
using namespace testing;
using namespace std::chrono_literals;

namespace {

using Test_Network_Server = Test_Network_Default;

//-----------------------------------------------------------------------------
TEST_P(Test_Network_Server, Listen)
{
	auto task = [&]() -> Task<>
	{
		[[maybe_unused]] auto server = co_await Server::listen(this->address());
	}();

	EXPECT_NO_THROW(async::waitResult(std::ref(task)));
}


TEST_P(Test_Network_Server, AddressInUse)
{
	auto task = [&]() -> Task<>
	{
		[[maybe_unused]] auto server1 = co_await Server::listen(this->address());
		[[maybe_unused]] auto server2 = co_await Server::listen(this->address());
	}();

	EXPECT_THROW(async::waitResult(std::ref(task)), NetworkException);
}


TEST_P(Test_Network_Server, AcceptWithDelay)
{ // In this test case connection should be pending at moment when accept is called.
	auto serverTask = [&]() -> Task<bool>
	{
		auto server = co_await Server::listen(this->address());
		co_await 50ms;
		[[maybe_unused]] auto stream = co_await server->accept();
		co_return static_cast<bool>(stream);
	}();

	auto connectTask = Client::connect(this->address());

	EXPECT_NO_THROW(async::waitResult(std::ref(connectTask)));
	EXPECT_NO_THROW(async::waitResult(std::ref(serverTask)));
	ASSERT_TRUE(serverTask.result());
}


TEST_P(Test_Network_Server, AcceptWithDelayBeforeConnect)
{ // In this test case when accept is calling there is no pending connection and it should be appear some later.
	TaskSource<> signal;

	auto serverTask = [&]() -> Task<bool>
	{
		auto server = co_await Server::listen(this->address());

		signal.resolve();

		[[maybe_unused]] auto stream = co_await server->accept();
		co_return static_cast<bool>(stream);
	}();

	async::waitResult(signal.getTask());

	std::this_thread::sleep_for(50ms);

	auto connectTask = Client::connect(this->address());

	EXPECT_NO_THROW(async::waitResult(std::ref(connectTask)));
	EXPECT_NO_THROW(async::waitResult(std::ref(serverTask)));
	ASSERT_TRUE(serverTask.result());
}


TEST_P(Test_Network_Server, DisposeFromNonRuntimeThread)
{
	Server::Ptr server = waitResult(Server::listen(this->address()));
}

TEST_P(Test_Network_Server, DisposeFromRuntimeThread)
{
	auto task = [](std::string address) -> Task<>
	{
		SWITCH_RUNTIME_SCHEDULER
		Server::Ptr server = co_await Server::listen(address);
	}
	(address());


	EXPECT_NO_THROW(async::waitResult(std::ref(task)));

}

TEST_P(Test_Network_Server, BreakAcceptOnDispose)
{
	Server::Ptr server = waitResult(Server::listen(this->address()));

	[&]() -> Task<>
	{
		co_await 20ms;
		server->dispose();
	}().detach();

	Stream::Ptr client = waitResult(server->accept());
	ASSERT_FALSE(client);
}

TEST_P(Test_Network_Server, BreakAcceptOnRuntimeShutdown)
{
	threading::Event signal;

	auto task = [&]() -> Task<>
	{
		Server::Ptr server = co_await Server::listen(this->address());
		signal.set();
		Stream::Ptr client = co_await server->accept();
	}();

	signal.wait();
	resetRuntime();
	
	EXPECT_NO_THROW(async::waitResult(std::ref(task)));
}

TEST_P(Test_Network_Server, ListenWithoutAccept)
{
	using namespace cold::async;

	using namespace std::chrono;

	constexpr size_t ClientCount = 1;


	TaskSource<> signal;

	std::vector<Task<Stream::Ptr>> clientTasks;

	

	for (size_t i = 0; i < ClientCount; ++i)
	{
		clientTasks.emplace_back(Client::connect(this->address()));
	}


	// TaskSource<> signal;

	//auto clientsTask = [&]() -> Task<>
	//{
	//	std::vector<Task<>> tasks;
	//	for (auto i = 0; i < ClientCount; ++i)
	//	{
	//		// attempt to connect some clients.
	//		// when any client is connected set signal to close server.
	//		// So some clients must connect, but other connect attempts will fail.
	//		auto task = [](std::string address, milliseconds delay) -> Task<>
	//		{
	//			co_await delay;
	//			LOG_debug_("CONNECTED0");
	//			[[maybe_unused]] auto c = co_await Client::connect(std::move(address), 10ms);
	//			LOG_debug_("CONNECTED1");
	//			
	//		}(this->address(), milliseconds(i * 7));

	//		tasks.emplace_back(std::move(task));
	//	}

	//	co_await async::whenAny(tasks);
	//	LOG_debug_("ANY READY");
	//	signal.resolve();
	//	co_await async::whenAll(tasks);
	//}();

	signal.resolve();

	auto serverTask = [&]() -> Task<>
	{
		using_(server) = co_await Server::listen(this->address());
		co_await signal.getTask();
		co_await 20ms;
	}();

	auto task = async::whenAll(clientTasks);


	EXPECT_NO_THROW(async::waitResult(std::ref(serverTask)));
	EXPECT_NO_THROW(async::waitResult(std::ref(task)));
}


}


INSTANTIATE_TEST_CASE_P(
	Default,
	Test_Network_Server,
	testing::ValuesIn(Test_Network_Base::addresses())
);