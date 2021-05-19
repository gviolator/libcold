#include "pch.h"
#include "cold/diagnostics/exception.h"
#include "cold/utils/strings.h"
#include "cold/remoting/httpparser.h"



namespace cold {


namespace {

std::string_view firstLine(std::string_view buffer)
{
	if (const auto end = buffer.find(HttpParser::EndOfLine); end != std::string_view::npos)
	{
		return {buffer.data(), end};
	}

	return {};
}

//
//template<typename T>
//std::string_view matchAsStringView(const T& iter)
//{
//	std::string_view str{ &(*iter.first), static_cast<size_t>(iter.length()) };
//
//	return str;
//}

#if 0
template<typename F>
void iteratePath(std::string_view pathString, F f)
{
	std::string_view current;

	do
	{
		if (current = HttpParser::Path::nextSegment(pathString, current); !current.empty())
		{
			if (!f(current))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	while (true);
}


template<typename F>
void iterateArguments(std::string_view argumentsString, F f)
{
	HttpParser::Argument current;

	do
	{
		if (current = HttpParser::Arguments::nextArgument(argumentsString, current); current)
		{
			if (!f(current))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	while (true);
}
#endif

}

//-----------------------------------------------------------------------------
HttpParser::Header::Header() = default;

HttpParser::Header::Header(std::string_view key_, std::string_view value_) : key(key_), value(value_)
{}

HttpParser::Header::operator bool() const
{
	return !key.empty();
}

bool HttpParser::Header::operator == (std::string_view key_) const
{
	return strings::icaseEqual(this->key, key_);
}

bool HttpParser::Header::operator == (const Header& other) const
{
	return this->operator == (other.key);
}

bool HttpParser::Header::operator != (const Header& other) const
{
	return !(this->operator == (other.key));
}

//-----------------------------------------------------------------------------
#if 0

std::string_view HttpParser::Path::nextSegment(std::string_view buffer, std::string_view current)
{
	return splitNext(buffer, current, "/?\r\n");
}


HttpParser::Path::Path() = default;

HttpParser::Path::Path(std::string_view path_) : m_path(path_)
{
}


HttpParser::Path::operator bool() const
{
	return !m_path.empty();
}


std::string_view HttpParser::Path::operator * () const
{
	return m_path;
}


size_t HttpParser::Path::size() const
{
	size_t counter = 0;

	iteratePath(m_path, [&counter](std::string_view)
	{
		++counter;

		return true;
	});

	return counter;
}


std::string_view HttpParser::Path::lastSegment() const
{
	std::string_view result;

	iteratePath(m_path, [&result](std::string_view segment)
	{
		result = segment;

		return true;
	});

	return result;
}


std::string_view HttpParser::Path::operator[](size_t index) const
{
	size_t counter = 0;

	std::string_view result;

	iteratePath(m_path, [&counter, &result, index](std::string_view segment)
	{
		if (counter == index)
		{
			result = segment;

			return false;
		}

		++counter;

		return true;
	});


	DEBUG_CHECK(!result.empty(), "Invalid http path segment index:(%1), actual segments count: (%2)", index, counter)

	return result;
}


HttpParser::Path::iterator HttpParser::Path::operator[](std::string_view target) const
{
	for (auto segment = begin(); segment != end(); ++segment)
	{
		if (*segment == target)
		{
			return segment;
		}
	}

	return end();
}



HttpParser::Path::iterator HttpParser::Path::begin() const
{
	return {*this, nextSegment(m_path,{})};
}


HttpParser::Path::iterator HttpParser::Path::end() const
{
	return {};
}


HttpParser::Path::iterator::iterator() = default;


HttpParser::Path::iterator::iterator(const Path& path_, std::string_view value_)
	: path(&path_)
	, value(value_)
{
}


bool HttpParser::Path::iterator::operator == (const iterator& other) const
{
	return this->path == other.path && this->value.data() == other.value.data();
}


bool HttpParser::Path::iterator::operator != (const iterator& other) const
{
	return this->path != other.path || this->value.data() != other.value.data();
}


HttpParser::Path::iterator& HttpParser::Path::iterator::operator++()
{
	DEBUG_CHECK(path && !value.empty(), "Invalid Path::iterator operation")

	if (value = nextSegment(path->m_path, value); value.empty())
	{
		path = nullptr;
	}

	return *this;
}


HttpParser::Path::iterator HttpParser::Path::iterator::operator++(int)
{
	DEBUG_CHECK(path && !value.empty(), "Invalid Path::iterator operation")

	auto iter{*this};

	this->operator++();

	return iter;
}


std::string_view HttpParser::Path::iterator::operator*() const
{
	DEBUG_CHECK(path && !value.empty(), "Invalid Path::iterator operation")

	return value;
}
#endif

//-----------------------------------------------------------------------------

#if 0

HttpParser::Argument::Argument() = default;


HttpParser::Argument::Argument(std::string_view line) : m_line(line)
{}


std::string_view HttpParser::Argument::operator * () const
{
	return m_line;
}


std::string_view HttpParser::Argument::key() const
{
	if (m_line.empty())
	{
		return {};
	}

	[[maybe_unused]] auto [key_, value_] = splitToArray<2>(m_line, "=");

	return trim(key_);
}


std::string_view HttpParser::Argument::value() const
{
	if (m_line.empty())
	{
		return {};
	}

	[[maybe_unused]] auto [key_, value_] = splitToArray<2>(m_line, "=");

	return trim(value_);

}


HttpParser::Argument::operator bool() const
{
	return !m_line.empty();
}


bool HttpParser::Argument::operator == (const Argument& other) const
{
	return icaseEq(key(), other.key()) && (value() == other.value());
}


bool HttpParser::Argument::operator != (const Argument& other) const
{
	return !icaseEq(key(), other.key()) || (value() != other.value());
}


HttpParser::Argument HttpParser::Arguments::nextArgument(std::string_view str, HttpParser::Argument current)
{
	auto line = splitNext(str, *current, "&\r\n");

	if (line.empty())
	{
		return {};
	}

	return Argument{line};
}


HttpParser::Arguments::Arguments() = default;


HttpParser::Arguments::Arguments(std::string_view arguments) : m_argumentsString(arguments)
{}


HttpParser::Arguments::operator bool() const
{
	return !m_argumentsString.empty();
}


std::string_view HttpParser::Arguments::operator * () const
{
	return m_argumentsString;
}


size_t HttpParser::Arguments::size() const
{
	size_t counter = 0;

	iterateArguments(m_argumentsString, [&counter](const Argument&)
	{
		++counter;

		return true;
	});

	return counter;
}


HttpParser::Argument HttpParser::Arguments::operator[](size_t index) const
{
	Argument result;

	size_t counter = 0;

	iterateArguments(m_argumentsString, [&counter, &result, index](const Argument& arg)
	{
		if (counter == index)
		{
			result = arg;

			return false;
		}

		++counter;

		return true;
	});

	DEBUG_CHECK(counter >= index, "Invalid http argument index:(%1), actual count: (%2)", index, counter)

	return result;
}


std::string_view HttpParser::Arguments::operator[](std::string_view key) const
{
	Argument result;

	iterateArguments(m_argumentsString, [key, &result](const Argument& arg)
	{
		if (icaseEq(arg.key(), key))
		{
			result = arg;

			return true;
		}

		return false;
	});

	return result.value();
}


HttpParser::Arguments::iterator HttpParser::Arguments::begin() const
{
	auto firstArgument = nextArgument(m_argumentsString, {});

	if (!firstArgument)
	{
		return {};
	}

	return iterator{*this, firstArgument};
}


HttpParser::Arguments::iterator HttpParser::Arguments::end() const
{
	return {};
}


HttpParser::Arguments::iterator::iterator() = default;


HttpParser::Arguments::iterator::iterator(const Arguments& arguments_, Argument value_)
	: arguments(&arguments_)
	, value(value_)
{}


bool HttpParser::Arguments::iterator::operator == (const iterator& other) const
{
	return this->arguments == other.arguments && (*this->value).data() == (*other.value).data();
}


bool HttpParser::Arguments::iterator::operator != (const iterator& other) const
{
	return this->arguments != other.arguments || ((*this->value).data() != (*other.value).data());
}

HttpParser::Arguments::iterator& HttpParser::Arguments::iterator::operator++()
{
	DEBUG_CHECK(arguments && value, "Invalid Arguments::iterator operation")


	if (value = nextArgument(arguments->m_argumentsString, value); !value)
	{
		arguments = nullptr;
	}

	return *this;
}


HttpParser::Arguments::iterator HttpParser::Arguments::iterator::operator++(int)
{
	DEBUG_CHECK(arguments && value, "Invalid Arguments::iterator operation")

	auto iter{*this};

	this->operator++();

	return iter;
}


HttpParser::Argument HttpParser::Arguments::iterator::operator*() const
{
	DEBUG_CHECK(arguments && value, "Invalid Arguments::iterator operation")

	return value;
}
#endif

//-----------------------------------------------------------------------------
#if 0

std::tuple<std::string_view, std::string_view> HttpParser::Request::splitRequest(std::string_view request)
{
	if (request.empty())
	{
		return {};
	}

	auto pathAndArgs = splitToArray<2>(request, "?");

	return {pathAndArgs[0], pathAndArgs[1]};
}

HttpParser::Request::Request() = default;


HttpParser::Request::Request(RequestData data)
	: m_method(data.method)
	, m_request(data.pathAndQuery)
{
}


HttpParser::Request::operator bool() const
{
	return !m_method.empty() || !m_request.empty();
}


std::string_view HttpParser::Request::method() const
{
	return m_method;
}


HttpParser::Path HttpParser::Request::path() const
{
	[[maybe_unused]] const auto [path_, args] = splitRequest(m_request);

	return path_;
}


HttpParser::Arguments HttpParser::Request::arguments() const
{
	[[maybe_unused]] const auto [path_, args] = splitRequest(m_request);

	return {args};
}
#endif
//-----------------------------------------------------------------------------
HttpParser::HeaderIterator::HeaderIterator() = default;

HttpParser::HeaderIterator::HeaderIterator(const HttpParser& parser_, const Header& value_): parser(&parser_), value(value_)
{}


bool HttpParser::HeaderIterator::operator == (const HeaderIterator& other) const
{
	return value == other.value;
}


bool HttpParser::HeaderIterator::operator != (const HeaderIterator& other) const
{
	return !(value == other.value);
}


HttpParser::HeaderIterator& HttpParser::HeaderIterator::operator++()
{
	DEBUG_CHECK(value, "Invalid http header iterator operation")
	DEBUG_CHECK(parser)

	if (value = HttpParser::nextHeader(parser->m_buffer, value); !value)
	{
		parser = nullptr;
	}

	return *this;
}


HttpParser::iterator HttpParser::HeaderIterator::operator++(int)
{
	auto iter{*this};

	this->operator++();

	return iter;
}

const HttpParser::Header& HttpParser::HeaderIterator::operator*() const
{
	DEBUG_CHECK(value, "Invalid http header iterator operation")
	return (value);
}


const HttpParser::Header* HttpParser::HeaderIterator::operator->() const
{
	DEBUG_CHECK(value, "Invalid http header iterator operation")
	return &value;
}

//-----------------------------------------------------------------------------

std::string_view HttpParser::getHeadersBuffer(std::string_view buffer)
{
	if (const auto pos = buffer.find(EndOfHeaders); pos != std::string_view::npos)
	{
		return std::string_view{buffer.data(), pos + EndOfHeaders.size()};
	}

	return {};
}


HttpParser::Header HttpParser::parseHeader(std::string_view line)
{
	if (line.empty())
	{
		return {};
	}

	constexpr auto Separator = ":";

	const auto keyValue = strings::split(line, Separator);
	auto key = keyValue.begin();
	auto value = key;
	if (++value == keyValue.end())
	{
		if (strings::endWith(strings::trimEnd(line), ":"))
		{
			return {strings::trim(*key), std::string_view{}};
		}

		LOG_error_("HTTP: Invalid header:(%1)", line)
		throw Excpt_("HTTP: Invalid header:(%1)", line);
	}

	return {strings::trim(*key), strings::trim(*value)};
}


std::string_view HttpParser::nextLine(std::string_view buffer, std::string_view line)
{
	if (line.empty())
	{
		return {};
	}

	DEBUG_CHECK(!buffer.empty())

	const ptrdiff_t offset = line.data() - buffer.data();

	DEBUG_CHECK(offset >= 0 && offset < static_cast<ptrdiff_t>(buffer.size()))

	const auto end1 = buffer.find(EndOfLine, static_cast<size_t>(offset));

	if (end1 == std::string_view::npos)
	{
		return {};
	}

	const auto begin = end1 + EndOfLine.size();
	const auto end = buffer.find(EndOfLine, begin);

	if (end == std::string_view::npos)
	{
		return {};
	}

	return buffer.substr(begin, end - begin);
}


HttpParser::Header HttpParser::firstHeader(std::string_view buffer)
{
	if (buffer.empty())
	{
		return {};
	}

	return parseHeader(nextLine(buffer, buffer));
}


HttpParser::Header HttpParser::nextHeader(std::string_view buffer, const Header& header)
{
	if (!header)
	{
		return {};
	}

	return parseHeader(nextLine(buffer, header.key));
}


HttpParser::Header HttpParser::findHeader(std::string_view buffer, std::string_view key)
{
	auto header = firstHeader(buffer);

	while (header)
	{
		if (header == key)
		{
			return header;
		}

		header = nextHeader(buffer, header);
	}


	return {};
}


size_t HttpParser::headersCount(std::string_view buffer)
{
	size_t counter = 0;

	for ([[maybe_unused]] auto header : HttpParser(buffer))
	{
		++counter;
	}

	return counter;
}


HttpParser::Header HttpParser::headerAt(std::string_view buffer, size_t index)
{
	size_t counter = 0;

	for ([[maybe_unused]] auto header : HttpParser(buffer))
	{
		if (counter == index)
		{
			return {header.key, header.value};
		}

		++counter;
	}

	RUNTIME_FAILURE("Invalid header index: (%1), headersCount: (%2)", index, counter)

	return {};
}

#if 0
std::optional<HttpParser::ResponseStatus> HttpParser::parseStatus(std::string_view buffer)
{
	constexpr auto Prefix = "HTTP/1.1";
	constexpr auto PrefixLength = sizeof(Prefix);

	auto line = firstLine(buffer);

	if (line.size() < PrefixLength) // HTTP CODE
	{
		LOG_Warning("HTTP buffer too short")
		return std::nullopt;
	}

	// "eat" 'HTTP/1.1';
	if (!icaseEq(line.substr(0, PrefixLength), Prefix))
	{
		return std::nullopt;
	}

	const auto [codeStart, codeEnd] = [line]() -> std::tuple<size_t, size_t>
	{
		constexpr auto NotFound = std::tuple {std::string_view::npos, std::string_view::npos};

		const size_t start = line.find(' ', 5);
		if (start == std::string_view::npos)
		{
			return NotFound;
		}

		const size_t end = line.find(' ', start + 1);

		return end == std::string_view::npos ? NotFound : std::tuple{start + 1, end};
	}();

	if (codeStart == std::string_view::npos)
	{
		return std::nullopt;
	}

	int code;
	if (const auto res = std::from_chars(line.data() + codeStart, line.data() + codeEnd, code); res.ptr != line.data() + codeEnd || res.ec != std::errc{})
	{
		return std::nullopt;
	}

	const char* const messageStart = line.data() + codeEnd + 1;
	const size_t messageLength = line.size() - codeEnd - 1;

	std::string_view message = messageLength == 0 ? std::string_view{} : std::string_view {messageStart, messageLength};

	return ResponseStatus {code, strings::trim(message)};
}


std::optional<HttpParser::RequestData> HttpParser::parseRequestData(std::string_view buffer)
{
	std::string_view line = firstLine(buffer);

	if (line.empty())
	{
		return std::nullopt;
	}

	const std::regex regex(R"(^([\w]+)\s+(.+)\s+HTTP\/(?:\d+\.\d+)$)", std::regex_constants::ECMAScript | std::regex_constants::icase);

	std::match_results<std::string_view::const_iterator> match;

	if (!std::regex_match(line.begin(), line.end(), match, regex))
	{
		return std::nullopt;
	}

	DEBUG_CHECK(match.size() > 2)

	auto method = matchAsStringView(match[1]);

	auto resource = trimEnd(matchAsStringView(match[2]));

	return RequestData{method, resource};
}
#endif


HttpParser::HttpParser() = default;


HttpParser::HttpParser(std::string_view buffer) : m_buffer(HttpParser::getHeadersBuffer(buffer))
{
	if (m_buffer.empty())
	{
		return;
	}

	DEBUG_CHECK(m_buffer.size() <= buffer.size())

	// std::string contentLengthStr((*this)["Content-Length"]);

	// m_contentLength = contentLengthStr.empty() ? size_t{0} : static_cast<size_t>(std::stoul(contentLengthStr));
}


HttpParser::operator bool() const
{
	return !m_buffer.empty();
}


size_t HttpParser::headersLength() const
{
	return m_buffer.length();
}


size_t HttpParser::contentLength() const
{
	if (!m_contentLength)
	{
		if (auto contentLengthValue = (*this)["Content-Length"]; !contentLengthValue.empty())
		{
			const std::string str{contentLengthValue};
			m_contentLength = static_cast<size_t>(std::stoul(str));
		}
		else
		{
			m_contentLength = 0;
		}
	}

	return *m_contentLength;
}


std::string_view HttpParser::operator[](std::string_view key) const
{
	return findHeader(m_buffer, key).value;
}


HttpParser::HeaderIterator HttpParser::begin() const
{
	return {*this, firstHeader(m_buffer)};
}


HttpParser::HeaderIterator HttpParser::end() const
{
	return {};
}

#if 0
std::optional<HttpParser::ResponseStatus> HttpParser::status() const
{
	return parseStatus(m_buffer);
}


std::optional<HttpParser::RequestData> HttpParser::requestData() const
{
	return parseRequestData(m_buffer);
}


HttpParser::Request HttpParser::request() const
{
	if (const auto data = requestData(); data)
	{
		return Request(*data);
	}

	return {};
}
#endif

} // namespace cold
