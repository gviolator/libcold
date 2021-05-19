#pragma once
#include <type_traits>

namespace cold::meta {

template<typename T, typename V = T>
struct RepresentationBase
{
	using Type = std::remove_const_t<T>;
	using ValueType = std::remove_const_t <V>;
};

}
