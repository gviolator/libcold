#include "pch.h"
#include "cold/memory/allocator.h"
#include "cold/com/comclass.h"

#include <corecrt_malloc.h>


namespace cold {


class CrtAllocator final : public Allocator
{
	COMCLASS_(Allocator)

public:
	~CrtAllocator()
	{
		std::cout << "~CrtAllocator done\n";
	}

	void* realloc(void* prevPtr, size_t size, std::optional<size_t>) override
	{
		return ::_aligned_realloc(prevPtr, size, sizeof(void*));
	}
	/**
	*/
	void free(void* ptr, std::optional<size_t>) override
	{
		::_aligned_free(ptr);
	}
};


namespace {

ComPtr<Allocator> defaultAllocator__;

} // namespace

ComPtr<Allocator> crtAllocator()
{
	static ComPtr<CrtAllocator> crtAllocator__ = com::createInstanceSingleton<CrtAllocator>();
	return crtAllocator__;
}

ComPtr<Allocator> defaultAllocator()
{
	return defaultAllocator__ ? defaultAllocator__ : crtAllocator();
}

void setDefaultAllocator(ComPtr<Allocator> allocator_)
{
	defaultAllocator__ = std::move(allocator_);
}

//-----------------------------------------------------------------------------
AllocatorMemoryResource::AllocatorMemoryResource(AllocatorProvider allocProvider): m_allocatorProvider(allocProvider)
{
	DEBUG_CHECK(m_allocatorProvider)
}

void* AllocatorMemoryResource::do_allocate(size_t size_, size_t align_)
{
	return allocator().alloc(size_, align_);
}

void AllocatorMemoryResource::do_deallocate(void* ptr_, size_t size_, size_t align_)
{
	allocator().free(ptr_, size_);
}

bool AllocatorMemoryResource::do_is_equal(const std::pmr::memory_resource& other_) const noexcept
{
	if (static_cast<const std::pmr::memory_resource*>(this) == &other_)
	{
		return true;
	}

	if (const AllocatorMemoryResource* const wrapper = dynamic_cast<const AllocatorMemoryResource*>(&other_); wrapper)
	{
		return &wrapper->allocator() == &this->allocator();
	}
		
	return false;
}

Allocator& AllocatorMemoryResource::allocator() const
{
	Allocator* const alloc_ = m_allocatorProvider();
	DEBUG_CHECK(alloc_)

	return *alloc_;
}

} // namespace cold
