#include "pch.h"
#include <cold/utils/cancellationtoken.h>

using namespace cold;
using namespace std::chrono_literals;


void someOp(Expiration token)
{
	token.isExpired();
}

TEST(Test_CancellationToken, Test1)
{
	auto cancellation = CancellationTokenSource::create();

	Expiration expiration = 10ms;


	ASSERT_FALSE(expiration.isExpired());

	std::this_thread::sleep_for(30ms);

	ASSERT_TRUE(expiration.isExpired());

	Cancellation token1{cancellation->token()};
	Cancellation token2 = token1;

	someOp(15ms);
	someOp(token2);

	Cancellation token13 = cancellation->token();

	token2 = {};
}


