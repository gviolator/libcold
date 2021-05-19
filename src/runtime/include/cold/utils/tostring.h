#pragma once
#include <string>
#include <type_traits>

namespace cold {

namespace internal__ {


template<typename T, typename A>
concept Convertible = std::is_convertible_v<T, A>;

}


template<typename T>
concept StringRepresentable = requires(const T &value) {
	{ toString(value) } -> internal__::Convertible<std::string>;
};


template<typename T>
concept WStringRepresentable = requires(const T &value) {
	{ toWString(value) } -> internal__::Convertible<std::wstring>;
};



template<typename T, typename C>
concept StringRepresentableT =
	(std::is_same_v<C, char> && StringRepresentable<T>) ||
	(std::is_same_v<C, wchar_t> && WStringRepresentable<T>)
	;



template<typename C, typename T>
inline std::basic_string<C> toStringT(const T& value) requires StringRepresentableT<T,C> {
	if constexpr (std::is_same_v<C, char>) {
		return toString(value);
	}
	else {
		return toWString(value);
	}
}

}
