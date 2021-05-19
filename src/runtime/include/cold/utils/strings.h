#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/meta/concepts.h>

#include <algorithm>
#include <ctype.h>
#include <iterator>
#include <type_traits>
#include <string_view>


namespace cold::strings_internal {

template<typename T>
constexpr inline auto choose([[maybe_unused]] const wchar_t* wstr, [[maybe_unused]] const char* str)// -> std::conditional_t<std::is_same_v<T, wchar_t>, const wchar_t*, const char*>
{
	if constexpr (std::is_same_v<T, wchar_t>)
	{
		return wstr;
	}
	else
	{
		return str;
	}
}

template<typename T>
constexpr inline auto choose([[maybe_unused]] wchar_t wchr, [[maybe_unused]] char chr)// -> std::conditional_t<std::is_same_v<T, wchar_t>, wchar_t, char>
{
	if constexpr (std::is_same_v<T, wchar_t>)
	{
		return wchr;
	}
	else
	{
		return chr;
	}
}

std::string_view RUNTIME_EXPORT splitNext(std::string_view str, std::string_view current, std::string_view separators);

std::wstring_view RUNTIME_EXPORT splitNext(std::wstring_view str, std::wstring_view current, std::wstring_view separators);


template<typename C>
struct SplitSequence
{
	class iterator : public std::iterator<std::forward_iterator_tag, std::basic_string_view<C>>
	{
	public:

		iterator() = default;

		iterator(std::basic_string_view<C> str_, std::basic_string_view<C> separators_, std::basic_string_view<C> current_ = {})
			: m_str(str_)
			, m_separators(separators_)
			, m_current(current_)
		{}

		bool operator == (const iterator& other) const
		{
			if (m_current.empty() || other.m_current.empty())
			{
				return m_current.empty() && other.m_current.empty();
			}

			return m_current.data() == other.m_current.data();
		}

		bool operator != (const iterator& other) const
		{
			return !this->operator==(other);
		}

		iterator& operator++()
		{
			if (m_current = strings_internal::splitNext(m_str, m_current, m_separators); m_current.empty())
			{
				m_str = {};
				m_separators = {};
			}

			return *this;
		}

		iterator operator++(int)
		{
			auto iter = *this;
			this->operator++();

			return iter;
		}

		std::basic_string_view<C> operator*() const
		{
			//DEBUG_CHECK(!m_current.empty(), L"Iterator is not dereferencible")
			return m_current;
		}

		std::basic_string_view<C>* operator-> () const
		{
			//DEBUG_CHECK(!m_current.empty(), L"Iterator is not dereferencible")
			return &m_current;
		}

	private:

		std::basic_string_view<C> m_str;
		std::basic_string_view<C> m_separators;
		std::basic_string_view<C> m_current;
	};

	std::basic_string_view<C> str;
	std::basic_string_view<C> separators;

	SplitSequence() = default;

	SplitSequence(std::basic_string_view<C> str_, std::basic_string_view<C> separators_): str(str_), separators(separators_)
	{}

	iterator begin() const
	{
		std::basic_string_view<C> next = strings_internal::splitNext(str, {}, separators);
		return next.empty() ? end() : iterator{str, separators, next};
	}

	iterator end() const
	{
		return {};
	}
};

template<typename C>
auto split__(std::basic_string_view<C> text, std::basic_string_view<C> separators)
{
	return SplitSequence<C>(text, separators);
}

} // namespace cold::strings_internal


namespace cold::strings {

inline char lower(char ch)
{
	const auto res = tolower(static_cast<unsigned char>(ch));
	return static_cast<char>(res);
}

inline char upper(char ch)
{
	const auto res = toupper(static_cast<unsigned char>(ch));
	return static_cast<char>(res);
}

inline bool isUpper(char ch)
{
	return isupper(static_cast<int>(ch)) != 0;
}

inline bool isLower(char ch)
{
	return islower(static_cast<int>(ch)) != 0;
}

inline wchar_t lower(wchar_t wch)
{
	const auto res = towlower(static_cast<unsigned short>(wch));
	return static_cast<wchar_t>(res);
}

inline wchar_t upper(wchar_t wch)
{
	const auto res = towupper(static_cast<unsigned short>(wch));
	return static_cast<wchar_t>(res);
}

inline bool isUpper(wchar_t ch)
{
	return iswupper(ch) != 0;
}

inline bool isLower(wchar_t ch)
{
	return iswlower(ch) != 0;
}

template<typename T, typename U>
auto split(T text, U separators)
{
	return strings_internal::split__(std::basic_string_view{text}, std::basic_string_view{separators});
}


std::string_view RUNTIME_EXPORT trimEnd(std::string_view str);

std::wstring_view RUNTIME_EXPORT trimEnd(std::wstring_view str);

std::string_view RUNTIME_EXPORT trimStart(std::string_view str);

std::wstring_view RUNTIME_EXPORT trimStart(std::wstring_view str);

std::string_view RUNTIME_EXPORT trim(std::string_view str);

std::wstring_view RUNTIME_EXPORT trim(std::wstring_view str);

bool RUNTIME_EXPORT endWith(std::string_view str, std::string_view);

bool RUNTIME_EXPORT endWith(std::wstring_view str, std::wstring_view);

bool RUNTIME_EXPORT startWith(std::string_view str, std::string_view);

bool RUNTIME_EXPORT startWith(std::wstring_view str, std::wstring_view);

//template<size_t N, typename T, typename Chars>
//auto splitToArray(std::basic_string_view<T> str, Chars separators) requires std::is_constructible_v<std::basic_string_view<T>, Chars>
//{
//	static_assert(N > 0);
//
//	std::array<std::basic_string_view<T>, N> result;
//	std::basic_string_view<T> current;
//
//	for (size_t i = 0; i < result.size(); ++i)
//	{
//		current = splitNext(str, current, separators);
//		if (current.empty())
//		{
//			break;
//		}
//
//		result[i] = current;
//	}
//
//	return result;
//}


/// <summary>
/// Perform 'basic' case insensitive string comparison.
/// </summary>
template<typename It1, typename It2>
inline bool icaseEqual(It1 begin1, It1 end1, It2 begin2, It2 end2) noexcept
{
	const auto len1 = std::distance(begin1, end1);
	const auto len2 = std::distance(begin2, end2);

	if (len1 != len2)
	{
		return false;
	}

	auto iter1 = begin1;
	auto iter2 = begin2;

	while (iter1 != end1)
	{
		const auto ch1 = *iter1;
		const auto ch2 = *iter2;

		if (lower(ch1) != lower(ch2))
		{
			return false;
		}

		++iter1;
		++iter2;
	}

	return true;
}

template<typename It1, typename It2>
inline int icaseCompare(It1 begin1, It1 end1, It2 begin2, It2 end2) noexcept
{
	static_assert(std::is_same_v<decltype(*begin1), decltype(*begin2)>, "String type mismatch");

	auto p = std::mismatch(begin1, end1, begin2, end2, [](auto c1, auto c2)
	{
		return lower(c1) == lower(c2);
	});

	if (p.first == end1)
	{
		return p.second == end2 ? 0 : -1;
	}
	else if (p.second == end2)
	{
		return 1;
	}

	const auto ch1 = lower(*p.first);
	const auto ch2 = lower(*p.second);

//	DEBUG_CHECK(ch1 != ch2)

	return ch1 < ch2 ? -1 : 1;
}


inline bool icaseEqual(std::string_view str1, std::string_view str2)
{
	return icaseEqual(std::begin(str1), std::end(str1), std::begin(str2), std::end(str2));
}


inline bool icaseEqual(std::wstring_view str1, std::wstring_view str2)
{
	return icaseEqual(std::begin(str1), std::end(str1), std::begin(str2), std::end(str2));
}



///// <summary>
///// Perform 'basic' case insensitive string equality check.
///// </summary>
//template<typename Str1, typename Str2>
//inline bool icaseEqal(const Str1& str1, const Str2& str2) noexcept
//{
//	using StringView = StringViewOf<Str1>;
//
//	static_assert(std::is_same_v<StringView, StringViewOf<Str2>>, "Character type mismatch");
//
//	return caseInsensitiveEqual(StringView{str1}, StringView{str2});
//}




template<typename Char>
int icaseCompare(std::basic_string_view<Char> str1, std::basic_string_view<Char> str2) noexcept
{
	return icaseCompare(std::begin(str1), std::end(str1), std::begin(str2), std::end(str2));
}

///// <summary>
///// Perform 'basic' case insensitive string equality check.
///// </summary>
//template<typename Str1, typename Str2>
//inline int icaseCompare(const Str1& str1, const Str2& str2) noexcept
//{
//	using Char1 = CharTypeOf<Str1>;
//
//	using Char2 = CharTypeOf<Str2>;
//
//	static_assert(std::is_same_v<Char1, Char2>, "Character type mismatch");
//
//	using StringView = std::basic_string_view<Char1>;
//
//	static_assert(std::is_constructible_v<StringView, const Str1&>, "Can not construct std::basic_string_view<> from specified type");
//
//	static_assert(std::is_constructible_v<StringView, const Str2&>, "Can not construct std::basic_string_view<> from specified type");
//
//	return caseInsensitiveCompare(StringView{str1}, StringView{str2});
//}



template<typename C = char>
struct CiStringComparer final
{
	using is_transparent = int;

	bool operator()(std::basic_string_view<C> str1, std::basic_string_view<C> str2) const noexcept
	{
		return icaseCompare(str1, str2) < 0;
	}
};

} // namespace cold::strings

#define TYPED_STR(T,text) cold::strings_internal::choose<T>(L##text, text)
#define TYPED_CHR(T,chr) cold::strings_internal::choose<T>(L##chr, chr)
