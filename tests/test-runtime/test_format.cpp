#include "pch.h"
#include <cold/utils/strfmt.h>
#include <cold/utils/stringconv.h>
#include <cold/memory/rtstack.h>
#include <cold/threading/barrier.h>

using namespace testing;
using namespace cold;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace {


struct Formatable {

private:
	friend std::string toString(const Formatable&)
	{
		return "ok";
	}

	friend std::wstring toWString(const Formatable&)
	{
		return L"ok";
	}
};


template<typename ... T>
AssertionResult check(const std::initializer_list<std::string_view>& formatStrings, std::string_view expectedString, bool expectSame, const T&...args)
{
	const std::wstring expectedWString = strings::wstringFromUtf8(expectedString);

	for (std::string_view formatStr : formatStrings)
	{
		const std::string str = strfmt(formatStr, args ...);
		if (expectSame != (str == expectedString))
		{
			return expectSame ?
				testing::AssertionFailure() << "[" << expectedString << "] expected be equal [" << str << "], while format [" << formatStr << "]" :
				testing::AssertionFailure() << "[" << expectedString << "] expected NOT be equal [" << str << "], while format [" << formatStr << "]"
				;
		}

		const std::wstring formatWStr = strings::wstringFromUtf8(formatStr);
		const std::wstring wStr = strfmt(formatWStr, args ...);
		if (expectSame != (wStr == expectedWString))
		{
			return expectSame ?
				testing::AssertionFailure() << L"[" << expectedWString << L"] expected be equal [" << wStr << L"], while format [" << formatWStr << "]" :
				testing::AssertionFailure() << L"[" << expectedWString << L"] expected NOT be equal [" << wStr << L"], while format [" << formatWStr << L"]"
				;

		}
	}

	return testing::AssertionSuccess();
}


template<typename ... T>
AssertionResult checkEq(const std::initializer_list<std::string_view>& formatStrings, std::string_view expectedString, const T&...args)
{
	return check(formatStrings, expectedString, true, args ...);
}

template<typename ... T>
AssertionResult checkNotEq(const std::initializer_list<std::string_view>& formatStrings, std::string_view expectedString, const T&...args)
{
	return check(formatStrings, expectedString, false, args ...);
}

}


TEST(Test_Common_Format, NoArgs)
{
	const std::string_view Text("test string");
	ASSERT_TRUE(check({Text}, Text, true));
}

TEST(Test_Common_Format, NoArgsIngoreFormatting)
{
	const std::string_view Text1= R"(
	"obj" : {
		"arg" : {"name" : "name1", "value": 1}
		}
	)";

	const std::string_view Text2("test string %1_%2");

	ASSERT_TRUE(checkEq({Text1}, Text1));
	ASSERT_TRUE(checkEq({Text2}, Text2));
}

TEST(Test_Common_Format, Simple)
{
	ASSERT_TRUE(checkEq({"one:{0}, two:{1}", "one:%1, two:%2"}, "one:1, two:22", 1, "22"));
}

TEST(Test_Common_Format, InvalidIndex)
{
	ASSERT_TRUE(checkNotEq({"_{0}_{1}_{1}"}, "_75_{1}_{1}", 75));
	ASSERT_TRUE(checkNotEq({"_%1_%2_%2"}, "_75_%2_%2", 75));
}

TEST(Test_Common_Format, UseToString)
{
	ASSERT_TRUE(checkEq({"{0}{1}{2}", "%1%2%3"}, "100ok200", 100, Formatable{}, 200, 300, 400));
}

TEST(Test_Common_Format, SkipPercent)
{
	ASSERT_THAT(strfmt("%%1", 777), Eq("%1"));
	ASSERT_THAT(strfmt("%%%1", 777), Eq("%777"));
	ASSERT_THAT(strfmt("%%", 777), Eq("%"));
}

TEST(Test_Common_Format, EmptyString)
{
	constexpr std::string_view EmptyString = "";
	constexpr std::wstring_view EmptyWString = L"";

	ASSERT_THAT(strfmt(EmptyString, 1, 2, 3), Eq(EmptyString));
	ASSERT_THAT(strfmt(EmptyWString, 1, 2, 3), Eq(EmptyWString));
	ASSERT_THAT(strfmt(EmptyString), Eq(EmptyString));
	ASSERT_THAT(strfmt(EmptyWString), Eq(EmptyWString));
}

TEST(Test_Common_Format, Pointers)
{
	const wchar_t* NullWChar =  nullptr;
	const char* NullChar = nullptr;
	const int* NullInt = (int*)5;

	if constexpr (sizeof(void*) == 8)
	{
		ASSERT_TRUE(checkEq({ "Nothing({0}{1})={2}", "Nothing(%1%2)=%3" }, "Nothing()=0000000000000005", NullWChar, NullChar, NullInt));
	}
}


TEST(Test_Common_Format, Benchmark)
{
	using namespace cold::cold_literals;

	constexpr size_t ThreadsCount = 5;
	
	auto runBench = [](const size_t threadsCount, const size_t iterCount, bool useRuntimeStack) -> std::chrono::milliseconds
	{
		std::vector<std::thread> threads;
		threading::Barrier barrier(threadsCount + 1);

		for (size_t x = 0; x < threadsCount; ++x)
		{
			threads.emplace_back([&]
			{
				std::optional<RtStackGuard> rtStack;
				if (useRuntimeStack)
				{
					rtStack.emplace(1_Mb);
				}

				barrier.enter();
				for (size_t i = 0; i < iterCount; ++i)
				{
					[[maybe_unused]] auto str = cold::strfmt("{0}{1}{2}: {3}, {4} Long long {5} string", "100ok200", 100, Formatable{}, i);
				}
			});
		}

		barrier.enter();
		const auto t1 = std::chrono::system_clock::now();

		for (auto& t : threads)
		{
			t.join();
		}

		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - t1);
	};

	const size_t t1 = runBench(ThreadsCount, 7'000, false).count();
	const size_t t2 = runBench(ThreadsCount, 7'000, true).count();

	//std::cout << "NO Runtime stack: " << t1 << " ms\n";
	//std::cout << "USE Runtime stack: " << t2 << " ms\n";
	//const float o = static_cast<float>(t1) / 100.0f;
	//std::cout << "diff: " << (static_cast<float>(t1) - static_cast<float>(t2)) / o  << " %\n";

	EXPECT_THAT(t1, Gt(t2));
}

