#include "pch.h"
#include <cold/remoting/httpparser.h>
#include <cold/utils/strings.h>


using namespace cold;
using namespace testing;

#if 0
TEST(Network_HttpParser, ParseRequest)
{
	constexpr std::string_view ExpectedMethod = "POST";
	constexpr std::string_view ExpectedResource = "/test/resource request/1";

	const auto line = format("%1 %2 HTTP/1.1\r\n", ExpectedMethod, ExpectedResource);

	const auto requestData = HttpParser::parseRequestData(line);
	ASSERT_TRUE(requestData);

	const auto [method, resource] = *requestData;

	ASSERT_THAT(method, Eq(ExpectedMethod));
	ASSERT_THAT(resource, Eq(ExpectedResource));
}


TEST(Network_HttpParser, FailParseRequest)
{
	const auto requestData = HttpParser::parseRequestData("HTTP/1.1 200 OK \r\n");

	ASSERT_FALSE(requestData);
}


TEST(Network_HttpParser, ParseStatus)
{
	constexpr int ExpectedCode = 404;
	constexpr auto ExpectedMessage = "Resource not found";

	const auto line = format("HTTP/1.1 %1 %2 \r\n", ExpectedCode, ExpectedMessage);

	const auto responseStatus = HttpParser::parseStatus(line);
	ASSERT_TRUE(responseStatus);

	const auto [code, message] = *responseStatus;

	ASSERT_THAT(code, Eq(ExpectedCode));
	ASSERT_THAT(message, Eq(ExpectedMessage));
}


TEST(Network_HttpParser, ParseStatusWithNoMessage)
{
	const int expectedCode = 404;

	const auto lineIllformed = format("HTTP/1.1 %1\r\n", expectedCode); // must not be valid, space before phrase is required: https://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html
	const auto line2 = format("HTTP/1.1 %1 \r\n", expectedCode);

	ASSERT_FALSE(HttpParser::parseStatus(lineIllformed));

	const auto status2 = HttpParser::parseStatus(line2);

	ASSERT_TRUE(status2);
	const auto [code, message] = *status2;

	ASSERT_THAT(code, Eq(expectedCode));
	ASSERT_TRUE(message.empty());
}

TEST(Network_HttpParser, FailParseStatus)
{
	const auto responseStatus = HttpParser::parseStatus("POST / HTTP/1.1 \r\n");

	ASSERT_FALSE(responseStatus);
}
#endif


#if 0

namespace  {

template<size_t N>
static void checkPath(HttpParser::Path path, std::array<std::string_view, N> expectedSegments)
{
	ASSERT_THAT(path.size(), Eq(expectedSegments.size()));

	if (path.size() == 0)
	{
		return;
	}

	size_t index = 0;

	ASSERT_FALSE(path.begin() == path.end());

	// check iteration
	for (auto segment : path)
	{
		ASSERT_THAT(segment, Eq(expectedSegments[index++]));
	}

	// check indexed access
	for (size_t i = 0, count = path.size(); i < count; ++i)
	{
		ASSERT_THAT(path[i], Eq(expectedSegments[i]));
	}

	ASSERT_THAT(path.lastSegment(), Eq(expectedSegments[expectedSegments.size() - 1]));
}

}


TEST(Network_HttpParser, PathWithNoArguments)
{
	constexpr std::string_view buffer("POST /first/hub/negotiate HTTP/1.1\r\n");

	const HttpParser::Request request(*HttpParser::parseRequestData(buffer));

	ASSERT_NO_FATAL_FAILURE(checkPath(request.path(), std::array<std::string_view,3>{"first", "hub", "negotiate"}));

	ASSERT_FALSE(request.arguments());
}


TEST(Network_HttpParser, PathWithArguments)
{
	const std::array<HttpParser::Argument, 3> expectedArguments = {
		HttpParser::Argument("id=91e1a008-667e-4d26-9bc7-504da2d4deb5"),
		HttpParser::Argument("arg1=value1"),
		HttpParser::Argument("arg2=")
	};

	constexpr std::string_view buffer("POST /first/hub/negotiate?id=91e1a008-667e-4d26-9bc7-504da2d4deb5&arg1 = value1&arg2= HTTP/1.1\r\n");

	const HttpParser::Request request(*HttpParser::parseRequestData(buffer));

	ASSERT_NO_FATAL_FAILURE(checkPath(request.path(), std::array<std::string_view,3>{"first", "hub", "negotiate"}));

	const auto arguments = request.arguments();

	ASSERT_THAT(arguments.size(), Eq(expectedArguments.size()));

	size_t index = 0;

	for (auto argument : arguments)
	{
		ASSERT_THAT(argument, Eq(expectedArguments[index++]));
	}

	for (size_t i = 0, count = arguments.size(); i < count; ++i)
	{
		ASSERT_THAT(arguments[i], Eq(expectedArguments[i]));
	}
}


TEST(Network_HttpParser, ParsePacket)
{
	constexpr std::string_view PacketBuffer = "HTTP/1.1 400 \r\nContent-Type: application/json\r\nInstance-Id: FileMetaRenderer-ac6c4f3\r\nInvoke-Id: renderFileMeta-d37b30\r\nContent-Length: 48\r\n\r\n{\r\n  \"typeName\": \"NotFoundException\",\r\n}";

	/*R"(HTTP/1.1 400
Content-Type: application/json
Instance-Id: FileMetaRenderer-2bc28b3
Invoke-Id: renderFileMeta-3bccd8
Content-Length: 402

{
	"typeName": "NotFoundException",
	"message": "",
	"origin": {
		"Item1": "   at eSignal.FileShare.HttpSessionManager.<Request>d__27.MoveNext() in E:\\Projects\\winsig_11-trunk\\externals\\esignal_packages\\fileshare\\eSignal.FileShare.Client\\HttpSessionManager.cs:line 425\r",
		"Item2": 0
	},
	"data": {
		"Uid": "404",
		"InvocationLevel": "",
		"Component": ""
	}
})";*/

	auto status = HttpParser::parseStatus(PacketBuffer);
}
#endif

TEST(Network_HttpParser, ParseHeaderLine)
{
	auto [key1, value1] = HttpParser::parseHeader("Key1 : Value1 ");
	ASSERT_THAT(key1, Eq("Key1"));
	ASSERT_THAT(value1, Eq("Value1"));

	auto [key2, value2] = HttpParser::parseHeader("Key2 :");
	ASSERT_THAT(key2, Eq("Key2"));
	ASSERT_TRUE(value2.empty());


	auto [key3, value3] = HttpParser::parseHeader(" Key3: ");
	ASSERT_THAT(key3, Eq("Key3"));
	ASSERT_TRUE(value3.empty());

	auto [key4, value4] = HttpParser::parseHeader("Letter:A");
	ASSERT_THAT(key4, Eq("Letter"));
	ASSERT_THAT(value4, Eq("A"));
}


TEST(Network_HttpParser, ParseHeaders)
{
	constexpr std::string_view buffer = "HTTP/1.1 400 \r\nContent-Type: application/json\r\nInstance-Id: FileMetaRenderer-ac6c4f3\r\nInvoke-Id: renderFileMeta-d37b30\r\nContent-Length: 48\r\n\r\n";

	const HttpParser parser(buffer);
	ASSERT_TRUE(parser);

	auto h1 = parser.begin();
	auto h2 = std::next(parser.begin(), 1);
	auto h3 = std::next(parser.begin(), 2);
	auto h4 = std::next(parser.begin(), 3);

	ASSERT_TRUE(h1 != parser.end());
	ASSERT_TRUE(h2 != parser.end());
	ASSERT_TRUE(h3 != parser.end());
	ASSERT_TRUE(h4 != parser.end());

	ASSERT_THAT(h1->key, Eq("Content-Type"));
	ASSERT_THAT(h1->value, Eq("application/json"));

	ASSERT_THAT(h2->key, Eq("Instance-Id"));
	ASSERT_THAT(h2->value, Eq("FileMetaRenderer-ac6c4f3"));

	ASSERT_THAT(h3->key, Eq("Invoke-Id"));
	ASSERT_THAT(h3->value, Eq("renderFileMeta-d37b30"));


	ASSERT_THAT(parser.contentLength(), Eq(48));
	ASSERT_THAT(++h4, Eq(parser.end()));
}


TEST(Network_HttpParser, FailParseHeaders)
{
	constexpr std::string_view buffer = "HTTP/1.1 400 \r\nContent-Type: application/json\r\n";
	const HttpParser parser(buffer);
	ASSERT_FALSE(parser);
}
