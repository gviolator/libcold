#include "pch.h"
#include <cold/diagnostics/runtimecheck.h>
#include <cold/memory/allocator.h>
#include <cold/memory/pageallocator.h>
#include <cold/memory/rtstack.h>
#include <cold/com/comclass.h>
#include <cold/threading/barrier.h>
#include <cold/threading/critical_section.h>

using namespace cold;
using namespace cold::cold_literals;
using namespace ::testing;


namespace {

class Test_PoolAllocator : public testing::Test
{
public:

protected:

	const Allocator::Ptr m_allocator = createPoolAllocator();
};

}


TEST_F(Test_PoolAllocator, Create)
{
	ASSERT_TRUE(m_allocator);
}


TEST_F(Test_PoolAllocator, SimpleAllocateAndFree)
{
	void* const ptr = m_allocator->alloc(60);
	ASSERT_THAT(ptr, NotNull());

	void* const ptr2 = m_allocator->alloc(512);
	m_allocator->free(ptr2);

	m_allocator->free(ptr);
}


TEST_F(Test_PoolAllocator, AllocateNothing)
{
	ASSERT_THAT(m_allocator->alloc(0), IsNull());
}


TEST_F(Test_PoolAllocator, FreeNothing)
{
	m_allocator->free(nullptr);
}


TEST_F(Test_PoolAllocator, Realloc)
{
	unsigned char* ptr = nullptr;

	for (unsigned char i = 0; i < 200; ++i) {

		ptr = reinterpret_cast<unsigned char*>(m_allocator->realloc(ptr, static_cast<size_t>(i+1)));
		ptr[i] = i;

		for (size_t j = 0; j <= i; ++j) {
			ASSERT_THAT(ptr[j], Eq(j));
		}
	}

	m_allocator->free(ptr);
}

#if 0

TEST_F(Test_PoolAllocator, FallbackToCrtAllocator)
{
	const size_t bigChunkSize = VMemory::pageSize() * 2;

	void* const ptr = m_allocator->alloc(bigChunkSize);
	ASSERT_THAT(ptr, NotNull());

	void* const ptr2 = m_allocator->realloc(ptr, bigChunkSize * 2);
	ASSERT_THAT(ptr2, NotNull());

	m_allocator->free(ptr2);
}

#endif


TEST_F(Test_PoolAllocator, ConcurrentAccess)
{
	constexpr size_t ThreadsCount = 15;
	constexpr size_t AllocationsCount = 15;

	std::list<std::thread> threads;
	const std::vector<size_t> allocationSizes({64, 128, 256, 512});

	const auto allocateManyTimes = [&]
	{
		std::list<void*> pointers;

		for (size_t i = 0; i < 100; ++i)
		{
			for (size_t size : allocationSizes)
			{
				void* const ptr = m_allocator->alloc(size);
				pointers.push_back(ptr);
			}
		}

		for (void* ptr : pointers)
		{
			m_allocator->free(ptr);
		}
	};

	threading::Barrier barrier(ThreadsCount);

	for (size_t i = 0; i < ThreadsCount; ++i)
	{
		threads.emplace_back([&]() -> void
		{
			barrier.enter();
			allocateManyTimes();
		});
	}


	std::for_each(threads.begin(), threads.end(), [](std::thread& t) { t.join(); });
}

static void runBench(Allocator::Ptr allocator)
{

	constexpr size_t Repeats = 500;

	const std::vector<size_t> allocationSizes({64, 128, 256, 512});
	// const std::vector<size_t> allocationSizes({512});

	const auto t1 = std::chrono::system_clock::now();

	using Ptr = void*;
	std::array<Ptr, 5000> pointers;
	size_t ptri = 0;



	for (size_t z = 0; z < Repeats; ++z)
	{

		ptri = 0;

		for (size_t i = 0; i < 500; ++i)
		{
			for (size_t size : allocationSizes)
			{
				pointers[ptri++] = allocator->alloc(size);
			}
		}

		for (size_t j = 0; j < ptri; ++j)
		{
			allocator->free(pointers[j]);
		}
	}

	std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - t1).count() << " ms\n";
}



TEST_F(Test_PoolAllocator, BenchMark)
{
	runBench(m_allocator);

	runBench(crtAllocator());
}


class PmrAllocatorWrapper : public std::pmr::memory_resource
{
public:

	PmrAllocatorWrapper(cold::Allocator* allocator_): m_allocator(allocator_)
	{}

private:

	void* do_allocate(size_t size_, size_t align_) override
	{
		return m_allocator->alloc(size_);
	}

	void do_deallocate(void* ptr_, size_t size_, size_t) override
	{
		m_allocator->free(ptr_, size_);
	}

	bool do_is_equal(const std::pmr::memory_resource& other_) const noexcept override
	{
		if (const PmrAllocatorWrapper* const wrapper = dynamic_cast<const PmrAllocatorWrapper*>(&other_); wrapper)
		{
			return wrapper->m_allocator == this->m_allocator;
		}

		return false;
	}


	cold::Allocator* const m_allocator;
};


TEST(Test_PoolAllocatorStd, Test1)
{
	auto alloc = createPoolAllocator();

	PmrAllocatorWrapper memRes{alloc.get()};


	std::pmr::list<uv_tcp_t> tcps{&memRes};
	std::pmr::list<uv_timer_t> timers{&memRes};

	tcps.emplace_back();
	timers.emplace_back();

}


struct MyStruct
{
	double value;
	int valuei;

	~MyStruct()
	{
		std::cout << "~MyStruct(" << value << ")\n";
	}
};

TEST(Test_RuntimeStack, Test1)
{
	rtstack(1_Mb);

	{
		rtstack();
		
		RtStackGuard::allocator().alloc(3);
		std::pmr::list<MyStruct> values(rtStackMemoryResource());

		values.emplace_back().value = 1.0;
		values.emplace_back().value = 2.0;

		{
			rtstack();
		
			RtStackGuard::allocator().alloc(3);

			using Str = std::pmr::string;

			std::pmr::list<Str> strs(rtStackMemoryResource());

			strs.emplace_back("124567");
			strs.emplace_back("890");

			for (const auto str : strs)
			{
				std::cout << str << std::endl;
			}

			LOG_debug_("ok")

		}

		LOG_debug_("ok")

	}

	{
		rtstack();
		
		RtStackGuard::allocator().alloc(3);

		using Str = std::pmr::string;

		std::pmr::list<Str> strs(rtStackMemoryResource());

		strs.emplace_back("2:124567");
		strs.emplace_back("2:890");

		for (const auto str : strs)
		{
			std::cout << str << std::endl;;
		}

		LOG_debug_("ok")

	}

}
