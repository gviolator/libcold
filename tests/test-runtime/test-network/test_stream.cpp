#include "pch.h"
#include "testnetworkbase.h"
#include "buffertestutils.h"
#include <cold/network/websocketcontrol.h>

#include <csignal> // set::signal lives here

using namespace cold;
using namespace cold::atd_literals;
using namespace cold::async;
using namespace cold::network;
using namespace testing;


using namespace std::chrono;

namespace {


//struct ExitHandler
//{
//	ExitHandler()
//	{
//		std::signal(SIGABRT, [](int)
//		{
//			if (IsDebuggerPresent() == TRUE)
//			{
//				DebugBreak();
//			}
//		});
//	}
//};



struct StreamTestParameter
{
	std::string address;
	size_t clientsCount = 1;
	size_t blocksCount = 1;
	cold::Byte blockSize = 1;
	milliseconds sendDelay = 0ms;
	milliseconds receiveDelay = 0ms;

	StreamTestParameter() = default;

	StreamTestParameter(std::string_view address_, size_t clientsCount_, size_t blocksCount_, Byte blockSize_, milliseconds sendDelay_, milliseconds receiveDelay_)
		: address(address_)
		, clientsCount(clientsCount_)
		, blocksCount(blocksCount_)
		, blockSize(blockSize_)
		, sendDelay(sendDelay_)
		, receiveDelay(receiveDelay_)
	{
	}

	StreamTestParameter(std::string_view address_, const StreamTestParameter& source)
		: StreamTestParameter(address_, source.clientsCount, source.blocksCount, source.blockSize, source.sendDelay, source.receiveDelay)
	{}

	StreamTestParameter(size_t clientsCount_, size_t blocksCount_, cold::Byte blockSize_, milliseconds sendingDelay_, milliseconds receiveDelay_)
		: StreamTestParameter("", clientsCount_, blocksCount_, blockSize_, sendingDelay_, receiveDelay_)
	{}

	template<typename T>
	friend std::ostream& operator << (std::ostream& outStream, const StreamTestParameter& params)
	{
		outStream << format("{0}: dataSize({1})", params.address, params.blocksCount);
		return outStream;
	}

	static std::vector<StreamTestParameter> makeSequence(const StreamTestParameter& param)
	{
		std::vector<StreamTestParameter> parameters;

		for (auto address : Test_Network_Base::addresses())
		{
			//if (includeTransport && std::find(includeTransport->begin(), includeTransport->end(), transport) == includeTransport->end())
			//{
			//	continue;
			//}

			//if (excludeTransport && std::find(excludeTransport->begin(), excludeTransport->end(), transport) != excludeTransport->end())
			//{
			//	continue;
			//}

			parameters.emplace_back(std::move(address), param);
		}

		return parameters;
	}

};

}


class Test_Network_StreamBasic : public Test_Network_Default
{};




TEST_P(Test_Network_StreamBasic, SendBuffer)
{
	auto originalBuffer = createBufferWithDefaultContent(256).toReadOnly();

	auto clientTask = [&]() -> Task<>
	{
		using_(client) = co_await Client::connect(this->address());
		co_await client->write(originalBuffer);
	}();


	auto serverTask = [&]() -> Task<ReadOnlyBuffer>
	{
		using_(server) = co_await Server::listen(this->address());
		Stream::Ptr client = co_await server->accept();

		auto data = co_await readEos(client);
		co_return data.toReadOnly();
	}();

	async::waitResult(std::ref(clientTask));
	auto resultBuffer = async::waitResult(std::ref(serverTask));

	ASSERT_TRUE(buffersEqual(originalBuffer, resultBuffer));
}


TEST_P(Test_Network_StreamBasic, SendStreamedBuffer)
{
	constexpr size_t BufferSize = 1024;
	constexpr size_t StreamedFrameSize = 200;
	constexpr size_t InboundBufferSize = 128;

	auto originalBuffer = createBufferWithDefaultContent(BufferSize).toReadOnly();

	auto clientTask = [&]() -> Task<>
	{
		using_(client) = co_await Client::connect(this->address());

		if (WebSocketControl* const control = client->as<WebSocketControl*>(); control)
		{
			control->setWriteStreamFrameSize(StreamedFrameSize);
		}

		co_await client->write(originalBuffer);
	}();


	auto serverTask = [&]() -> Task<ReadOnlyBuffer>
	{
		using_(server) = co_await Server::listen(this->address());
		Stream::Ptr client = co_await server->accept();
		if (SocketControl* const control = client->as<SocketControl*>(); control)
		{
			control->setInboundBufferSize(InboundBufferSize);
		}

		auto data = co_await readEos(client);
		co_return data.toReadOnly();
	}();

	async::waitResult(std::ref(clientTask));
	auto resultBuffer = async::waitResult(std::ref(serverTask));

	ASSERT_TRUE(buffersEqual(originalBuffer, resultBuffer));
}



TEST_P(Test_Network_StreamBasic, BrokenRead)
{
	auto originalBuffer = createBufferWithDefaultContent(128).toReadOnly();
	TaskSource<> signal;

	auto clientTask = [&]() -> Task<>
	{
		auto client = co_await Client::connect(this->address());
		co_await Test_Network_Base::readCount(client, originalBuffer.size());
		signal.resolve();
		auto buffer = co_await client->read();
		if (buffer.size() > 0)
		{
			throw Excpt_(L"Expected empty buffer");
		}
	}();


	auto serverTask = [&]() -> Task<>
	{
		Stream::Ptr client;
		{
			using_(server) = co_await Server::listen(this->address());
			client = co_await server->accept();
		}

		co_await client->write(originalBuffer);
		co_await signal.getTask();
		co_await 20ms;
	}();

	ASSERT_NO_THROW(async::waitResult(std::ref(serverTask)));
	ASSERT_NO_THROW(async::waitResult(std::ref(clientTask)));
}


TEST_P(Test_Network_StreamBasic, BrokenWrite)
{
	auto originalBuffer = createBufferWithDefaultContent(128).toReadOnly();
	TaskSource<> signal;

	auto clientTask = [&]() -> Task<>
	{
		{
			auto client = co_await Client::connect(this->address());
			co_await Test_Network_Base::readCount(client, originalBuffer.size());
		}
		co_await 60ms;
		signal.resolve();
	}();


	auto serverTask = [&]() -> Task<>
	{
		using_(server) = co_await Server::listen(this->address());
		using_(client) = co_await server->accept();

		co_await client->write(originalBuffer);
		co_await signal.getTask();
		for (size_t i = 0; i < 50; ++i)
		{
			co_await client->write(originalBuffer);
		}

	}();

	ASSERT_NO_THROW(async::waitResult(std::ref(serverTask)));
	ASSERT_NO_THROW(async::waitResult(std::ref(clientTask)));
}




class Test_Network_Stream : public Test_Network_Base, public testing::TestWithParam<StreamTestParameter>
{
protected:
	std::string address() const override
	{
		return GetParam().address;
	}
};




TEST_P(Test_Network_Stream, SendAndReceive)
{
	const StreamTestParameter& testParam = GetParam();
	const size_t ExpectedBufferSize = testParam.blocksCount * testParam.blockSize ;

	auto originalBuffer = createBufferWithDefaultContent(ExpectedBufferSize).toReadOnly();

	const auto spawnSender = [&](milliseconds delay) -> Task<>
	{
		//using_(client) = Client::create();

		if (delay > 0ms)
		{
			co_await delay;
		}

		using_(client) = co_await Client::connect(this->address());

		for (size_t i = 0; i < testParam.blocksCount; ++i)
		{
			const size_t offset = testParam.blockSize * i;
			co_await client->write({originalBuffer, offset, testParam.blockSize});
			if (testParam.sendDelay > 0ms)
			{
				co_await testParam.sendDelay;
			}
		}
	};


	const auto processReceiver = [&](Stream::Ptr stream) -> Task<AssertionResult>
	{
		SCOPE_Leave {
			stream->dispose();
		};

		BytesBuffer buffer;

		while (buffer.size() < ExpectedBufferSize)
		{
			if (testParam.receiveDelay > 0ms)
			{
				co_await testParam.receiveDelay;
			}

			auto inboundBuffer = co_await stream->read();
			if (inboundBuffer.size() == 0)
			{
				break;
			}

			buffer += std::move(inboundBuffer);
		}

		co_return buffersEqual(buffer.toReadOnly(), originalBuffer);
	};


	const auto spawnServer = [&]() -> Task<AssertionResult>
	{
		using_(server) = co_await Server::listen(this->address());

		std::list<Task<AssertionResult>> clients;

		for (size_t i = 0; i < testParam.clientsCount; ++i)
		{
			auto client = co_await server->accept();
			clients.emplace_back(processReceiver(std::move(client)));
		}

		co_await async::whenAll(clients);
		for (auto& client : clients)
		{
			if (auto assertRes = client.result(); !assertRes)
			{
				co_return assertRes;
			}
		}

		co_return AssertionSuccess();
	};

	auto serverTask = spawnServer();

	std::list<Task<>> senderTasks;
	for (size_t i = 0; i < testParam.clientsCount; ++i)
	{
		senderTasks.emplace_back(spawnSender(3ms));
	}
	
	EXPECT_NO_THROW(async::wait(serverTask));
	for (auto& task : senderTasks)
	{
		async::wait(task);
	}

	ASSERT_TRUE(serverTask.result());
};



INSTANTIATE_TEST_CASE_P(
	SingleClientNoDelay,
	Test_Network_Stream,
	testing::ValuesIn(StreamTestParameter::makeSequence({
		StreamTestParameter(1, 1, 1_Kb, 0ms, 0ms)
	}))
);

INSTANTIATE_TEST_CASE_P(
	MultiplyClientsNoDelay,
	Test_Network_Stream,
	testing::ValuesIn(StreamTestParameter::makeSequence({
		StreamTestParameter(20, 1, 1_Kb, 0ms, 0ms)
	}))
);

INSTANTIATE_TEST_CASE_P(
	MultiplyClientDelaySend,
	Test_Network_Stream,
	testing::ValuesIn(StreamTestParameter::makeSequence({
		StreamTestParameter(40, 10, 64_b, 5ms, 0ms)
	}))
);

INSTANTIATE_TEST_CASE_P(
	MultiplyClientDelayReceive,
	Test_Network_Stream,
	testing::ValuesIn(StreamTestParameter::makeSequence({
		StreamTestParameter(20, 10, 64_b, 0ms, 20ms)
	}))
);


INSTANTIATE_TEST_CASE_P(
	MultiplyClientDelaySendAndReceive,
	Test_Network_Stream,
	testing::ValuesIn(StreamTestParameter::makeSequence({
		StreamTestParameter(20, 5, 128_b, 2ms, 8ms)
	}))
);


INSTANTIATE_TEST_CASE_P(
	Default,
	Test_Network_StreamBasic,
	testing::ValuesIn(Test_Network_Base::addresses())
);
