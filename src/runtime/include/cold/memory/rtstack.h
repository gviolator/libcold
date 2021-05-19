#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/memory/allocator.h>
#include <cold/utils/preprocessor.h>

namespace cold {

class RUNTIME_EXPORT RtStackGuard
{
public:


	static Allocator& allocator();


	RtStackGuard();

	RtStackGuard(Kilobyte size);

	~RtStackGuard();

	RtStackGuard(const RtStackGuard&) = delete;

	RtStackGuard& operator = (const RtStackGuard&) = delete;


private:

	RtStackGuard* const m_prev = nullptr;
	Allocator::Ptr m_allocator;
	size_t m_top = 0;
};


RUNTIME_EXPORT Allocator& rtStack();

RUNTIME_EXPORT std::pmr::memory_resource* rtStackMemoryResource();


template<typename T>
using RtStackStdAllocator = StdAllocator<T, RtStackGuard>;

}


#define rtstack(...) const cold::RtStackGuard ANONYMOUS_VARIABLE_NAME(rtStack__) {__VA_ARGS__}

