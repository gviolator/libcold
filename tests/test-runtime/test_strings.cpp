#include "pch.h"
#include <cold/utils/strings.h>


using namespace cold;
using namespace testing;


TEST(Common_Strings, Split)
{
	auto seq = strings::split("word1, word2|word3 ", ",|");

	auto it1 = seq.begin();
	auto it2 = std::next(seq.begin(), 1);
	auto it3 = std::next(seq.begin(), 2);

	ASSERT_THAT(*it1, Eq("word1"));
	ASSERT_THAT(*it2, Eq(" word2"));
	ASSERT_THAT(*it3, Eq("word3 "));

	ASSERT_THAT(++it3, Eq(seq.end()));
}


TEST(Common_Strings, EndWith)
{
	ASSERT_TRUE(strings::endWith("Content-Length:", ":"));
	ASSERT_FALSE(strings::endWith("Content-Length:::_", ":::"));
	ASSERT_FALSE(strings::endWith("value", "1-value"));
}


TEST(Common_Strings, StartWith)
{
	ASSERT_TRUE(strings::startWith("Content-Length:", "Content-"));
	ASSERT_FALSE(strings::startWith("Content-Length:", " Content-"));
}
