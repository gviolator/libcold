#include "pch.h"
#include <cold/memory/membase.h>
#include <cold/memory/pageallocator.h>

using namespace cold;
using namespace cold::cold_literals;

using namespace testing;


TEST(Test_Membase, Literals)
{
	ASSERT_THAT(1_b.bytesCount(), Eq(1));
	ASSERT_THAT(1_Kb.bytesCount(), Eq(1024));
	ASSERT_THAT(1_Mb.bytesCount(), Eq(1024 * 1024));
}

TEST(Test_Membase, LiteralsOp)
{
	Kilobyte kb = 1_Mb;
	ASSERT_THAT(kb.bytesCount(), Eq(1024*1024));
}

TEST(Test_Pageallocator, Alloc)
{
	PageAllocator allocator(AllocationGranuarity);
	void* const ptr = allocator.alloc(PageSize);
	ASSERT_THAT(ptr, NotNull());

	void* const ptr2 = allocator.alloc(allocator.size());
	ASSERT_THAT(ptr2, IsNull());
}

TEST(Test_Pageallocator, AllocAligned)
{
	PageAllocator allocator(AllocationGranuarity);

	constexpr std::array alignments = {
		alignof(uint16_t),
		alignof(uint32_t),
		alignof(uint64_t),
		alignof(double),
		alignof(__m64),
		alignof(__m128)
	};

	for (auto value: alignments)
	{
		allocator.alloc(1);
		const size_t size = value;
		const size_t align = value;
		const ptrdiff_t addr = reinterpret_cast<ptrdiff_t>(allocator.alloc(size, align));
		ASSERT_THAT(addr % align, Eq(0));
	}
}
