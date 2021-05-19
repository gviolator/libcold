#include "pch.h"
#include "cold/utils/format.h"
#include "cold/utils/stringconv.h"
#include "cold/memory/rtstack.h"

#include <sstream>
#include <memory_resource>

#define FORMAT_ENABLE_CS_STYLE
#define FORMAT_ENABLE_QT_STYLE


namespace cold::atd_internal {


namespace {


inline bool isDigit(char ch)
{
	return isdigit(static_cast<int>(ch));
}

inline bool isDigit(wchar_t ch)
{
	return iswdigit(ch);
}


/// <summary>
///
/// </summary>
struct FormattingBase
{
	std::ios_base& io;
	const std::ios::fmtflags originalFlags;

	FormattingBase(const FormattingBase&) = delete;

	FormattingBase(std::ios_base& io_): io(io_), originalFlags(io_.flags())
	{}

	~FormattingBase()
	{
		io.flags(originalFlags);
	}
};

/// <summary>
///
/// </summary>
struct NumericBaseFormatting : FormattingBase
{
	template<typename T>
	static bool accepts(std::basic_string_view<T> format_)
	{
		const auto prefix = strings::lower(format_.front());

		return prefix == TYPED_STR(T,'x') || prefix == TYPED_STR(T,'d');
	}

	template<typename T>
	NumericBaseFormatting(std::basic_ostream<T>& stream_, std::basic_string_view<T> format_): FormattingBase(stream_)
	{
		const auto prefix = strings::lower(format_.front());

		if (prefix == TYPED_STR(T,'x'))
		{
			stream_ << std::hex;
		}
		else if (prefix == TYPED_STR(T,'d'))
		{
			stream_ << std::dec;
		}

		if (strings::isUpper(format_.front()))
		{
			stream_ << std::uppercase;
		}
	}
};

/// <summary>
///
/// </summary>
struct FFixedFormatting : FormattingBase
{
	const std::streamsize originalPrecision;

	template<typename T>
	static bool accepts(std::basic_string_view<T> format_)
	{
		return strings::lower(format_.front()) == TYPED_STR(T,'f');
	}

	template<typename T>
	FFixedFormatting(std::ios_base& stream_, std::basic_string_view<T> format): FormattingBase(stream_), originalPrecision(stream_.precision())
	{
		if (format.size() < 3)
		{
			throw std::exception("Bad {F} format");
		}

		const auto precision = std::stoi(std::basic_string<T>{format.data() + 2, format.size() - 2});

		stream_.precision(static_cast<std::streamsize>(precision));
		stream_.setf(std::ios::fixed, std::ios::floatfield);
	}

	~FFixedFormatting()
	{
		io.precision(originalPrecision);
	}
};


/// <summary>
///
/// </summary>
template<typename C>
class FormatContext
{
public:

	using Stream = std::basic_ostream<C>;

	FormatContext(Stream& stream_, std::basic_string_view<C> format_)
	{
		parseOptions(stream_, format_, m_options);
	}

private:

	using FromattingOptions = std::tuple<
		std::optional<NumericBaseFormatting>,
		std::optional<FFixedFormatting>
	>;

	static void parseOptions(Stream& stream, std::basic_string_view<C> formatting, FromattingOptions& options)
	{
		for (std::basic_string_view<C> format : strings::split(formatting, TYPED_STR(C,":")))
		{
			if (FFixedFormatting::accepts(format))
			{
				std::get<std::optional<FFixedFormatting>>(options).emplace(stream, format);
			}
			else if (NumericBaseFormatting::accepts(format))
			{
				std::get<std::optional<NumericBaseFormatting>>(options).emplace(stream, format);
			}
		}
	}

	FromattingOptions m_options;
};


/// <summary>
///
/// </summary>
template<typename C>
auto parseArgFormat(std::basic_string_view<C> content)
{
	std::basic_string_view<C> indexStr;
	std::basic_string_view<C> formatStr;

	if (const auto pos = content.find(TYPED_STR(C,":")); pos != std::basic_string_view<C>::npos)
	{
		indexStr = content.substr(0, pos);
		formatStr = content.substr(pos + 1, content.size() - pos - 1);
	}
	else
	{
		indexStr = content;
	}

	const std::basic_string<C> index{strings::trim(indexStr)};
	size_t idx;
	const int number = index.empty() ? -1 : std::stoi(index, &idx, 10);

	return std::tuple{number, strings::trim(formatStr)};
}


/// <summary>
///
/// </summary>
template<typename C>
std::basic_string<C> formatImpl(std::basic_string_view<C> text, const FormatArgumentEntry__<C>* args , size_t argsCount)
{
	if (argsCount == 0 || text.empty())
	{
		return std::basic_string<C>{text};
	}

	rtstack();


	constexpr C QFormatSymbol = TYPED_CHR(C, '%');
	constexpr C OpenBraceSymbol = TYPED_CHR(C, '{');
	constexpr C CloseBraceSymbol = TYPED_CHR(C, '}');

	std::basic_stringstream<C, std::char_traits<C>, RtStackStdAllocator<C>> stream;
	// std::basic_stringstream<C, std::char_traits<C>, std::pmr::polymorphic_allocator<C>> stream;

	// std::basic_stringstream<C> stream;
	std::optional<FormatContext<C>> defaultFormat;

	stream << std::boolalpha;

	size_t anchor = 0;
	size_t currentPos = 0;

	const size_t lastIndex = text.size() - 1;

	while (currentPos <= lastIndex)
	{
		C ch = text[currentPos];

		std::basic_string_view<C> formatContent;
		std::optional<int> argIndexOffset;

		const auto lastPos = currentPos;

		if (ch == OpenBraceSymbol)
		{
#ifdef FORMAT_ENABLE_CS_STYLE
			if (const auto closePos = text.find(CloseBraceSymbol, currentPos + 1); closePos != std::basic_string_view<C>::npos)
			{
				const size_t length = closePos - (currentPos + 1);

				formatContent = std::basic_string_view<C>{text.data() + lastPos + 1, length};
				currentPos = closePos + 1;
			}
#endif
		}
		else if (ch == QFormatSymbol && currentPos != lastIndex)
		{
#ifdef FORMAT_ENABLE_QT_STYLE
			size_t offset = currentPos + 1;

			if (text[offset] == QFormatSymbol)
			{
				stream << QFormatSymbol;
				currentPos = (offset + 1);
				anchor = currentPos;
				continue;
			}

			while (isDigit(text[offset]))
			{
				if (++offset == text.size())
				{
					break;
				}
			}
			
			if (formatContent = std::basic_string_view<C>{text.data() + lastPos + 1, offset - lastPos - 1}; !formatContent.empty())
			{
				argIndexOffset = -1;
				currentPos = offset;
			}
#endif
		}

		if (formatContent.empty())
		{
			++currentPos;
			continue;
		}

		if (std::basic_string_view<C> subStr {text.data() + anchor, lastPos - anchor}; !subStr.empty()) {
			stream << subStr;
		}

		anchor = currentPos;

		try
		{
			auto [index, format] = parseArgFormat(formatContent);

			if (index >= 0 && argIndexOffset)
			{
				index += *argIndexOffset;
			}

			if (index < 0)
			{
				defaultFormat.emplace(stream, format);
			}
			else if (index < argsCount)
			{
				FormatContext<C> ctx(stream, format);
				auto [writer, arg] = args[index];
				writer(stream, arg);
			}
			else
			{
				FormatContext<C> f(stream, TYPED_STR(C, "D"));
				stream << TYPED_STR(C, "INVALID ARG INDEX(") << index << TYPED_STR(C, ")");
			}
		}
		catch (const std::exception& exception)
		{
			stream << TYPED_STR(C, "ERROR:(") << exception.what() << TYPED_STR(C, ")");
		}
	}

	if (anchor < text.size())
	{
		if (anchor == 0)
		{
			return std::basic_string<C>{text};
		}

		std::basic_string_view<C> subStr {text.data() + anchor, text.size() - anchor};
		stream << subStr;
	}

	const auto res = stream.str();

	return std::basic_string { res.data(), res.length() };
}

} // namespace


std::string formatImpl__(std::string_view text, const FormatArgumentEntry__<char>* args, size_t argsCount)
{
	return formatImpl(text, args, argsCount);
}

std::wstring formatImpl__(std::wstring_view text, const FormatArgumentEntry__<wchar_t>* args, size_t argsCount)
{
	return formatImpl(text, args, argsCount);
}


} // namespace cold::atd_internal


//-----------------------------------------------------------------------------

namespace cold::formatters {

void formatValue(std::basic_ostream<char>& stream, std::wstring_view wstr)
{
	const std::string bytes =strings::toUtf8(wstr);
	stream << bytes;
}

void formatValue(std::basic_ostream<wchar_t>& stream, std::string_view str)
{
	const std::wstring wstr = strings::wstringFromUtf8(str);
	stream << wstr;
}


}
