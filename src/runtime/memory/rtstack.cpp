#include "pch.h"
#include "cold/com/comclass.h"
#include "cold/memory/rtstack.h"
#include "cold/memory/pageallocator.h"

namespace cold {

namespace {

RtStackGuard* currentStack(std::optional<RtStackGuard*> newValue = std::nullopt)
{
	static thread_local RtStackGuard* currentStack__ = nullptr;

	if (newValue)
	{
		RtStackGuard* const prev = currentStack__;
		currentStack__ = *newValue;
		return prev;
	}

	return currentStack__;
}

}


class RtStackAllocator final : public Allocator
{
	COMCLASS_(Allocator)

public:

	RtStackAllocator(const Kilobyte size): m_allocator(size.bytesCount())
	{}

	~RtStackAllocator()
	{
		// std::cout << "max rt size: " << m_allocator.allocatedSize() << std::endl;
	}

	void restore(size_t offset)
	{
		m_offset = offset;
	}

	size_t offset() const
	{
		return m_offset;
	}

private:

	void* realloc(void* prevPtr, size_t size, std::optional<size_t> alignment) override;

	void free(void* ptr, std::optional<size_t>) override;


	PageAllocator m_allocator;
	size_t m_offset = 0;
};


void* RtStackAllocator::realloc([[mayb_unused]] void* prevPtr, size_t size, std::optional<size_t> optionalAlignment)
{
	DEBUG_CHECK(!prevPtr)

	std::byte* topPtr = reinterpret_cast<std::byte*>(m_allocator.base()) + m_offset;

	const size_t alignment = optionalAlignment ? *optionalAlignment : alignof(std::max_align_t);
	DEBUG_CHECK(isPOT(alignment), "Alignment must be power of two")

	const size_t alignedBlockSize = alignedSize(size, alignment);

	// padding = offset from topPtr to make result address aligned.
	const size_t d = reinterpret_cast<ptrdiff_t>(topPtr) % alignment;
	const size_t padding = d == 0 ? 0 : (alignment - d); 

	const size_t allocationSize = alignedBlockSize + padding;

	if (const size_t newOffset = m_offset + allocationSize; newOffset <= m_allocator.allocatedSize())
	{
		m_offset = newOffset;
	}
	else
	{
		topPtr = reinterpret_cast<std::byte*>(m_allocator.alloc(allocationSize));
		RUNTIME_CHECK(topPtr, "Runtime dynamic stack is out of memory")
		m_offset = m_allocator.allocatedSize();
	}

	return topPtr + padding;
}

void RtStackAllocator::free([[maybe_unused]] void* ptr, [[maybe_unused]] std::optional<size_t>)
{ // DEBUG: need to check that ptr belongs to the current runtime stack allocation range.
}

//-----------------------------------------------------------------------------
RtStackGuard::RtStackGuard(): m_prev(currentStack(this))
{
	if (m_prev && m_prev->m_allocator)
	{
		m_allocator = m_prev->m_allocator;
		m_top = m_prev->m_allocator->as<RtStackAllocator&>().offset();
	}
}

RtStackGuard::RtStackGuard(Kilobyte size)
	: m_prev(currentStack(this))
	, m_allocator(com::createInstance<RtStackAllocator, Allocator>(size))
{
}

RtStackGuard::~RtStackGuard()
{
	if (m_allocator)
	{
		m_allocator->as<RtStackAllocator&>().restore(m_top);
	}

	RUNTIME_CHECK(currentStack(m_prev) == this);
}


Allocator& RtStackGuard::allocator()
{
	auto current = currentStack();
	Allocator* const allocator__ = (current && current->m_allocator) ? current->m_allocator.get() : crtAllocator().get();
	DEBUG_CHECK(allocator__)
	return *allocator__ ;
}

//-----------------------------------------------------------------------------

Allocator& rtStack()
{
	return RtStackGuard::allocator();
}

std::pmr::memory_resource* rtStackMemoryResource()
{
	static AllocatorMemoryResource memRes__([]() -> Allocator*
	{
		return &RtStackGuard::allocator();
	});

	return &memRes__;
}

//RUNTIME_EXPORT Allocator::Ptr rtStackAllocator();
//
}
