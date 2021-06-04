#pragma once
#include <optional>
#include <string_view>

namespace cold {


/// <summary>
/// see https://www.w3.org/Protocols/rfc2616/rfc2616.html
/// </summary>
class HttpParser
{
public:

	struct Header
	{
		std::string_view key;
		std::string_view value;

		Header();
		Header(std::string_view, std::string_view);
		explicit operator bool() const;
		bool operator == (const Header&) const;
		bool operator != (const Header&) const;
		bool operator == (std::string_view) const;
	};

#if 0
	class Path final
	{
	public:

		struct iterator final
		{
			using iterator_category = std::input_iterator_tag;

			using value_type = std::string_view;

			using difference_type = ptrdiff_t;


			const Path* path = nullptr;

			std::string_view value;


			iterator();

			iterator(const Path&, std::string_view);

			bool operator == (const iterator&) const;

			bool operator != (const iterator&) const;

			iterator& operator++();

			iterator operator++(int);

			std::string_view operator*() const;
		};

		static std::string_view nextSegment(std::string_view path, std::string_view currentSegment);


		Path();

		Path(std::string_view);

		explicit operator bool() const;

		std::string_view operator * () const;

		size_t size() const;

		std::string_view lastSegment() const;

		std::string_view operator[](size_t index) const;

		Path::iterator operator[](std::string_view) const;

		iterator begin() const;

		iterator end() const;

	private:

		std::string_view m_path;
	};


	class Argument final
	{
	public:

		Argument();

		explicit Argument(std::string_view line);

		std::string_view operator * () const;

		std::string_view key() const;

		std::string_view value() const;

		explicit operator bool() const;

		bool operator == (const Argument&) const;

		bool operator != (const Argument&) const;

	private:

		std::string_view m_line;
	};


	class Arguments final
	{
	public:

		struct iterator final
		{
			using iterator_category = std::input_iterator_tag;

			using value_type = Argument;

			using difference_type = ptrdiff_t;


			const Arguments* arguments = nullptr;

			Argument value;


			iterator();

			iterator(const Arguments&, Argument);

			bool operator == (const iterator&) const;

			bool operator != (const iterator&) const;

			iterator& operator++();

			iterator operator++(int);

			Argument operator*() const;
		};


		static Argument nextArgument(std::string_view str, Argument current);


		Arguments();

		Arguments(std::string_view);

		explicit operator bool() const;


		std::string_view operator * () const;

		size_t size() const;

		Argument operator[](size_t index) const;

		std::string_view operator[](std::string_view key) const;

		iterator begin() const;

		iterator end() const;

	private:

		std::string_view m_argumentsString;
	};



	/// <summary>
	/// (code, message)
	/// </summary>
	using ResponseStatus = std::tuple<int, std::string_view>;

	/// <summary>
	/// (method, resource path with parameters)
	/// </summary>
	struct RequestData
	{
		const std::string_view method;

		const std::string_view pathAndQuery;


		RequestData() = default;

		RequestData(std::string_view method_, std::string_view pathAndQuery_): method(method_), pathAndQuery(pathAndQuery_)
		{}

		RequestData(std::string_view pathAndQuery_): RequestData(std::string_view{}, pathAndQuery_)
		{};
	};


	class Request
	{
	public:

		Request();

		Request(RequestData);

		explicit operator bool() const;

		std::string_view method() const;

		Path path() const;

		Arguments arguments() const;

	private:

		static std::tuple<std::string_view, std::string_view> splitRequest(std::string_view request);


		std::string_view m_method;

		std::string_view m_request;
	};
#endif

	constexpr static std::string_view EndOfLine{"\r\n"};

	constexpr static std::string_view EndOfHeaders{ "\r\n\r\n" };



	struct HeaderIterator
	{
		using iterator_category = std::input_iterator_tag;
		using value_type = Header;
		using difference_type = ptrdiff_t;
		using pointer = Header*;
		using reference = Header&;

		const HttpParser* parser = nullptr;
		Header value;

		HeaderIterator();
		HeaderIterator(const HttpParser& parser, const Header&);
		bool operator == (const HeaderIterator&) const;
		bool operator != (const HeaderIterator&) const;
		HeaderIterator& operator++();
		HeaderIterator operator++(int);
		const Header& operator*() const;
		const Header* operator->() const;
	};

	using iterator = HeaderIterator;


	static std::string_view getHeadersBuffer(std::string_view buffer);

	static Header parseHeader(std::string_view line);

	static std::string_view nextLine(std::string_view buffer, std::string_view line);

	static Header firstHeader(std::string_view buffer);

	static Header nextHeader(std::string_view buffer, const Header& header);

	static Header findHeader(std::string_view buffer, std::string_view key);

	static size_t headersCount(std::string_view buffer);

	static Header headerAt(std::string_view buffer, size_t index);

	// static std::optional<ResponseStatus> parseStatus(std::string_view buffer);

	// static std::optional<RequestData> parseRequestData(std::string_view buffer);


	HttpParser();

	HttpParser(std::string_view buffer);

	explicit operator bool() const;

	size_t headersLength() const;

	size_t contentLength() const;

	std::string_view operator[](std::string_view) const;

	iterator begin() const;

	iterator end() const;

//	std::optional<ResponseStatus> status() const;

//	std::optional<RequestData> requestData() const;

//	Request request() const;

private:

	std::string_view m_buffer;

	mutable std::optional<size_t> m_contentLength;
};



}


namespace std {


inline auto begin(const cold::HttpParser& parser)
{
	return parser.begin();
}

inline auto end(const cold::HttpParser& parser)
{
	return parser.end();
}

//inline auto begin(const cold::HttpParser::Path& path)
//{
//	return path.begin();
//}
//
//inline auto end(const cold::network::HttpParser::Path& path)
//{
//	return path.end();
//}
//
//inline auto begin(const cold::network::HttpParser::Arguments& args)
//{
//	return args.begin();
//}
//
//inline auto end(const cold::network::HttpParser::Arguments& args)
//{
//	return args.end();
//}

}
