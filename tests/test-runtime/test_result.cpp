#include "pch.h"
#include <cold/utils/result.h>
#include <cold/utils/scopeguard.h>



using namespace cold;
using namespace testing;


namespace {

class SomeException : public std::exception
{
public:
	SomeException(const char* message = "some exception"): std::exception(message)
	{}
};

class CustomException : public std::exception
{
public:
	CustomException(const char* message = "sample failure"): std::exception(message)
	{}
};

class CodeException : public CustomException
{
	DECLARE_CLASS_BASE(CustomException)
public:
	CodeException(int code_): CustomException("Error code"), m_code(code_)
	{}

	int code() const {
		return m_code;
	}

private:
	const int m_code;
};

}





TEST(Common_Result, ConstructValueInplace) {
	const std::string_view ExpectedValue {"test string"};
	const Result<std::string> res(InplaceResult, ExpectedValue.data(), ExpectedValue.size());
	ASSERT_TRUE(res);
	ASSERT_THAT(*res, Eq(ExpectedValue));
}


TEST(Common_Result, HasException) {
	const Result<std::string> errorResult(InplaceError<CodeException>, 1);
	ASSERT_FALSE(errorResult);
	ASSERT_TRUE(errorResult.hasException<CodeException>());
	ASSERT_TRUE(errorResult.hasException<CustomException>());
	ASSERT_TRUE(errorResult.hasException<>());
	ASSERT_FALSE(errorResult.hasException<SomeException>());
}


TEST(Common_Result, HasExceptionPtr) {

	constexpr int ExpectedCode = 7;

	const Result<std::string> errorResult(std::make_exception_ptr(CodeException{ExpectedCode}));
	ASSERT_FALSE(errorResult);
	ASSERT_TRUE(errorResult.hasException<CodeException>());
	ASSERT_TRUE(errorResult.hasException<CustomException>());
	ASSERT_TRUE(errorResult.hasException<>());

	std::optional<int> actualCode;
	try {
		errorResult.rethrowIfException();
	}
	catch (const CodeException& codeException) {
		actualCode = codeException.code();
	}

	ASSERT_TRUE(actualCode);
	ASSERT_THAT(*actualCode, Eq(ExpectedCode));
}


TEST(Common_Result, HasNoException) {
	const Result<std::string> result(std::string{});
	ASSERT_FALSE(result.hasException());
}


TEST(Common_Result, CoroReturnTrivialValue) {

	const auto func1 = []() -> Result<unsigned> {
		co_return 64ui32;
	};

	const auto func2 = [&func1]() -> Result<int> {
		auto val1 = co_await func1();
		auto val2 = co_await func1();

		co_return val1 + val2;
	};

	Result<int> result = func2();

	ASSERT_TRUE(result);
	ASSERT_THAT(*result, Eq(128));
}


TEST(Common_Result, CoroReturnValue) {

	const auto concat = [](std::string_view str1, std::string_view str2) -> Result<std::string> {
		
		if (str1.empty()) {
			co_return str2;
		}
		else if (str2.empty())
		{
			co_return str1;
		}

		co_return std::string{str1} + std::string{str2};
	};

	const auto func2 = [concat]() -> Result<std::string> {
		auto val1 = co_await concat("abc", "def");
		auto val2 = co_await concat("123", "456");

		co_return (val1 + val2).c_str();
	};

	Result<std::string> result = func2();

	ASSERT_TRUE(result);
	ASSERT_THAT(*result, Eq("abcdef123456"));
}


TEST(Common_Result, CoroThrowException) {

	const auto pass = [](unsigned code) -> Result<int> {

		if (code == 0) {
			throw std::exception("error");
		}

		co_return code;
	};


	const auto func = [pass](int& accum) -> Result<int> {

		accum += co_await pass(1);
		accum += co_await pass(2);
		accum += co_await pass(0);
		accum += co_await pass(10);

		co_return accum;
	};

	int accum = 0;

	auto result = func(accum);

	ASSERT_FALSE(result);
	ASSERT_THAT(accum, Eq(3));
}


TEST(Common_Result, CoroDestructOnException) {

	const auto pass = [](unsigned code) -> Result<int> {
		return code == 0 ? Result<int>{std::exception("error")} : Result<int>{InplaceResult, code };
	};


	bool leaveScope1 = false;
	bool callAfterException = false;

	const auto func = [&]() -> Result<int> {

		SCOPE_Leave {
			leaveScope1 = true;
		};

		co_await pass(0);
		callAfterException = true;
		co_await pass(10);

		co_return 0;
	};


	auto result = func();

	ASSERT_FALSE(callAfterException);
	ASSERT_TRUE(leaveScope1);
	ASSERT_FALSE(result);
}


TEST(Common_Result, ResultVoidConstructWithSuccess) {
	Result<> res = success;
	ASSERT_TRUE(res);
}

TEST(Common_Result, ResultVoidConstructWithException) {
	Result<> res = Excpt_("Fail");
	ASSERT_FALSE(res);
	ASSERT_TRUE(res.hasException());
}

TEST(Common_Result, ResultVoidCoroReturn) {

	const auto func1 = []() -> Result<unsigned> {
		co_return 64ui32;
	};

	const auto func2 = [&func1]() -> Result<> {
		auto val1 = co_await func1();
		if (val1 == 0) {
			throw Excpt_("Fail");
		}
	};

	Result<> result = func2();

	ASSERT_TRUE(result);
}


TEST(Common_Result, ResultVoidCoroException) {

	const auto func1 = []() -> Result<unsigned> {
		co_return 0;
	};

	const auto func2 = [&func1]() -> Result<> {
		auto val1 = co_await func1();
		if (val1 == 0) {
			throw Excpt_("Fail");
		}
	};

	Result<> result = func2();

	ASSERT_FALSE(result);
}

