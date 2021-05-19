#pragma once
#include <cold/meta/concepts.h>

namespace cold {

template<typename T>
concept Lockable = requires (T& mtx)
{
	mtx.lock();
	mtx.unlock();
};

}
