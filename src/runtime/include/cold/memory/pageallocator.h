#pragma once

#include "membase.h"
#include <memoryapi.h>

namespace cold {

/*
* 
*/
class PageAllocator
{
public:

	PageAllocator(size_t size_): m_size(alignedSize(size_, AllocationGranuarity))
	{
		m_ptr = VirtualAlloc(nullptr, static_cast<SIZE_T>(m_size), MEM_RESERVE, PAGE_READWRITE);
	}

	~PageAllocator()
	{
		VirtualFree(m_ptr, 0, MEM_FREE);
	}

	void* alloc(size_t size) noexcept
	{
		const size_t newAllocOffset = m_allocOffset + size;
		auto bytePtr = reinterpret_cast<std::byte*>(m_ptr);

		if (newAllocOffset > m_commitedSize)
		{
			const size_t commitSize = alignedSize(newAllocOffset - m_commitedSize, PageSize);
			const size_t newCommitedSize = m_commitedSize + commitSize;
			if (newCommitedSize > m_size)
			{
				return nullptr;
			}

			[[maybe_unused]] const void* const unusedPtr = VirtualAlloc(bytePtr + m_commitedSize, commitSize, MEM_COMMIT, PAGE_READWRITE);
			m_commitedSize = newCommitedSize;
		}

		void* const ptr = bytePtr + m_allocOffset;
		m_allocOffset = newAllocOffset;
		return ptr;
	}

	void* alloc(size_t size, size_t alignment) noexcept
	{
		DEBUG_CHECK(isPOT(alignment), "Alignment must be power of two")

		const size_t alignedBlockSize = alignedSize(size, alignment);

		// make result address aligned:
		const size_t d = reinterpret_cast<ptrdiff_t>(reinterpret_cast<std::byte*>(m_ptr) + m_allocOffset) % alignment;
		const size_t padding = d == 0 ? 0 : (alignment - d); 

		std::byte* const ptr = reinterpret_cast<std::byte*>(this->alloc(alignedBlockSize + padding));

		return ptr == nullptr ? nullptr : ptr + padding;
	}

	bool contains(const void* ptr) const
	{
		return m_ptr <= ptr && ptr < (reinterpret_cast<std::byte*>(m_ptr) + m_size);
	}

	void* base() const
	{
		return m_ptr;
	}

	size_t size() const
	{
		return m_size;
	}

	size_t allocatedSize() const
	{
		return m_allocOffset;
	}

	size_t commitedSize() const
	{
		return m_commitedSize;
	}

private:
	const size_t m_size;
	size_t m_commitedSize = 0;
	size_t m_allocOffset = 0;
	void* m_ptr;
};

}
