#pragma once

#if __has_include (<coroutine>)

#include <coroutine>

#define STD_CORO std

#elif __has_include (<experimental/coroutine>)

#include <experimental/coroutine>

namespace std {

#define STD_CORO std::experimental

template <typename Promise = void>
using coroutine_handle = ::std::experimental::coroutine_handle<Promise>;

using suspend_never = ::std::experimental::suspend_never;

using suspend_always = ::std::experimental::suspend_always;

}


#endif
