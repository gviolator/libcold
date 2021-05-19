#include "pch.h"
#include "buffertestutils.h"
#include <cold/memory/bytesbuffer.h>
#include <cold/diagnostics/logging.h>
#include <cold/com/comclass.h>


using namespace testing;
using namespace cold;


/// <summary>
///
/// </summary>
template<typename T>
static testing::AssertionResult checkBufferDefaultContent(const T& buffer, size_t offset = 0, size_t contentOffset = 0, std::optional<size_t> size = std::nullopt)
{
	static_assert(
		std::is_same_v<T, BytesBuffer> ||
		std::is_same_v<T, ReadOnlyBuffer> ||
		std::is_same_v<T, BufferView>);

	const size_t fillSize = !size ? (buffer.size() - offset) : *size;

	RUNTIME_CHECK(offset + fillSize <= buffer.size());

	const uint8_t* const charPtr = reinterpret_cast<const uint8_t*>(buffer.data());

	for (size_t i = offset; i < fillSize; ++i)
	{
		const uint8_t expectedValue = static_cast<uint8_t>((i + contentOffset) % std::numeric_limits<uint8_t>::max());

		const uint8_t bufferValue = charPtr[i];

		if (bufferValue != expectedValue)
		{
			return testing::AssertionFailure() << format("Value mismatch at position [%1]. Expected [%2 , %3'], but [%4, '%5']", i, static_cast<int>(expectedValue), expectedValue, static_cast<int>(bufferValue), bufferValue);
		}
	}

	return testing::AssertionSuccess();
}


/// <summary>
///
/// </summary>
TEST(Test_Buffers, Emptiness)
{
	BytesBuffer emptyBuffer;
	ASSERT_FALSE(emptyBuffer);


	BytesBuffer emptyBuffer2;
	ASSERT_FALSE(emptyBuffer2);

	ReadOnlyBuffer emptyReadonlyBuffer;
	ASSERT_FALSE(emptyReadonlyBuffer);

	// BufferView emptyBufferView;
	// ASSERT_FALSE(emptyBufferView);

	BytesBuffer nonEmptyBuffer(10);
	ASSERT_TRUE(nonEmptyBuffer);
	ASSERT_THAT(BufferUtils::refsCount(nonEmptyBuffer), Eq(1));
	ASSERT_EQ(nonEmptyBuffer.size(), 10);


	ReadOnlyBuffer nonEmptyReadonlyBuffer(nonEmptyBuffer.toReadOnly());
	ASSERT_TRUE(nonEmptyReadonlyBuffer);
	ASSERT_THAT(BufferUtils::refsCount(nonEmptyReadonlyBuffer), Eq(1));
	ASSERT_THAT(nonEmptyReadonlyBuffer.size(), Eq(10));
	ASSERT_FALSE(nonEmptyBuffer);


	BufferView readOnlyBufferView(std::move(nonEmptyReadonlyBuffer));
	ASSERT_TRUE(readOnlyBufferView);
	ASSERT_THAT(readOnlyBufferView.size(), Eq(10));
	ASSERT_FALSE(nonEmptyReadonlyBuffer);
}

/// <summary>
///
/// </summary>
TEST(Test_Buffers, Content)
{
	constexpr size_t TestSize = 100;

	BytesBuffer buffer(TestSize);

	std::byte* basePointer = buffer.data();

	ASSERT_TRUE(buffer);
	ASSERT_EQ(buffer.size(), TestSize);

	fillBufferWithDefaultContent(buffer);

	{
		SCOPED_TRACE("Check buffer initial content");

		ASSERT_TRUE(checkBufferDefaultContent(buffer));
	}

	ReadOnlyBuffer readOnlyBuffer;

	readOnlyBuffer = std::move(buffer);

	ASSERT_TRUE(readOnlyBuffer);
	ASSERT_FALSE(buffer);

	const std::byte* readOnlyPointer = readOnlyBuffer.data();

	ASSERT_EQ(readOnlyPointer, basePointer);

	{
		SCOPED_TRACE("Check readonly buffer initial content");

		ASSERT_TRUE(checkBufferDefaultContent(readOnlyBuffer));
	}
}


/// <summary>
///
/// </summary>
TEST(Test_Buffers, Modify)
{
	constexpr size_t InitialSize = 50;

	BytesBuffer buffer1(InitialSize);
	fillBufferWithDefaultContent(buffer1);

	buffer1.resize(buffer1.size() * 2);
	{
		SCOPED_TRACE("Non unique buffer content should be unchanged after growing the size");
		ASSERT_TRUE(checkBufferDefaultContent(buffer1, 0, 0, InitialSize));
	}

	BytesBuffer buffer2(InitialSize);
	fillBufferWithDefaultContent(buffer2);

	buffer2.resize(InitialSize / 2);
	ASSERT_TRUE(checkBufferDefaultContent(buffer2));


	BytesBuffer buffer3(InitialSize);
	fillBufferWithDefaultContent(buffer3);

	std::byte* pointerBeforeResize = buffer3.data();

	// buffer3 has no additional references and its size going smaller, so we expect that its pointer remaining unchanged, only size should be changed.
	buffer3.resize(buffer3.size() / 2);

	std::byte* pointerAfterResize = buffer3.data();

	ASSERT_EQ(pointerBeforeResize, pointerAfterResize);

	{
		SCOPED_TRACE("Unique buffer content should be unchanged after reducing the size");
		ASSERT_TRUE(checkBufferDefaultContent(buffer3));
	}

	BytesBuffer buffer4(InitialSize);

	fillBufferWithDefaultContent(buffer4);

	buffer4.resize(buffer4.size() * 2);

	{
		SCOPED_TRACE("Unique buffer content should be unchanged after growing the size");
		ASSERT_TRUE(checkBufferDefaultContent(buffer4, 0, 0, InitialSize));
	}
}

/*
TEST(Test_Buffers, Merge)
{
	constexpr size_t InitialSize = 50;

	ReadOnlyBuffer readOnlyBuffer1{};
	ReadOnlyBuffer readOnlyBuffer2{};

	{
		BytesBuffer buffer1(InitialSize);
		BytesBuffer buffer2(InitialSize * 2);

		fillBufferWithDefaultContent(buffer1);
		fillBufferWithDefaultContent(buffer2);

		readOnlyBuffer1 = std::move(buffer1);
		readOnlyBuffer2 = std::move(buffer2);
	}

	{
		std::array<ReadOnlyBuffer, 2> buffers = {readOnlyBuffer1, readOnlyBuffer2};

		BytesBuffer mergedBuffer = mergeBuffers(buffers.begin(), buffers.end());

		ASSERT_EQ(mergedBuffer.size(), readOnlyBuffer1.size() + readOnlyBuffer2.size());

		SCOPED_TRACE("Check merged buffer");
		ASSERT_TRUE(checkBufferDefaultContent(mergedBuffer, 0, readOnlyBuffer1.size()));
		ASSERT_TRUE(checkBufferDefaultContent(BufferView(mergedBuffer.toReadOnly(), static_cast<ptrdiff_t>(readOnlyBuffer1.size()))));
	}

	BufferView bufferView1(readOnlyBuffer1, 0, readOnlyBuffer1.size() / 2);
	BufferView bufferView2(readOnlyBuffer1, readOnlyBuffer1.size() / 2);

	auto mergedView = BufferView::merge(bufferView1, bufferView2);

	ASSERT_EQ(mergedView.getBuffer(), bufferView1.getBuffer());
}
*/

TEST(Test_Buffers, Move)
{
	constexpr uint32_t InitialSize = 50ui32;

	BytesBuffer buffer(InitialSize);

	fillBufferWithDefaultContent(buffer);

	const std::byte* initialPtr = buffer.data();

	{
		ReadOnlyBuffer readOnlyBuffer = buffer.toReadOnly();

		ASSERT_FALSE(buffer);
		ASSERT_TRUE(readOnlyBuffer);
		ASSERT_EQ(readOnlyBuffer.data(), initialPtr);

		buffer = readOnlyBuffer.toBuffer();

		ASSERT_TRUE(buffer);
		ASSERT_FALSE(readOnlyBuffer);
		ASSERT_EQ(buffer.data(), initialPtr);
	}

	{
		const auto viewSize = buffer.size() / 2;

		BufferView view{buffer.toReadOnly(), 0, viewSize};
		ASSERT_FALSE(buffer);
		ASSERT_TRUE(view);
		ASSERT_EQ(view.data(), initialPtr);

		buffer = view.toBuffer();

		ASSERT_TRUE(buffer);
		ASSERT_FALSE(view);
		ASSERT_EQ(buffer.data(), initialPtr);
		ASSERT_EQ(buffer.size(), viewSize);
	}

	ASSERT_TRUE(checkBufferDefaultContent(buffer));

	ReadOnlyBuffer readOnlyBuffer = buffer.toReadOnly();
	ReadOnlyBuffer readOnlyBufferCopy = readOnlyBuffer;
	ASSERT_THAT(BufferUtils::refsCount(readOnlyBuffer) ,Eq(2));

	// Trying to move buffer that have references.
	// Internally copy operation should be performed, but in any case readOnlyBuffer will be released.
	// So test expects that there is only one one reference to the original buffer.
	buffer = readOnlyBuffer.toBuffer();
	ASSERT_FALSE(readOnlyBuffer);
	ASSERT_THAT(BufferUtils::refsCount(readOnlyBufferCopy), Eq(1));

	ASSERT_THAT(buffer.size(), Eq(readOnlyBufferCopy.size()));
	ASSERT_NE(buffer.data(), initialPtr);

	ASSERT_TRUE(checkBufferDefaultContent(buffer));
}


TEST(Test_Buffers, Copy)
{
	constexpr uint32_t InitialSize = 50;

	BytesBuffer buffer(InitialSize);
	fillBufferWithDefaultContent(buffer);


	BytesBuffer buffer2 = BufferUtils::copy(buffer);
	ASSERT_TRUE(checkBufferDefaultContent(buffer2));

	ASSERT_NE(buffer.data(), buffer2.data());
	ASSERT_EQ(buffer.size(), buffer2.size());

	BytesBuffer buffer3 = BufferUtils::copy(BufferView{buffer2.toReadOnly()});
	ASSERT_TRUE(checkBufferDefaultContent(buffer3));

	ASSERT_NE(buffer.data(), buffer3.data());
	ASSERT_EQ(buffer.size(), buffer3.size());
}


TEST(Test_Buffers, View)
{
	constexpr size_t InitialSize = 100;

	const auto initializeView = [InitialSize]() -> BufferView
	{
		BytesBuffer buffer{InitialSize};

		fillBufferWithDefaultContent(buffer);

		return {buffer.toReadOnly()};
	};

	BufferView view1 = initializeView();

	ASSERT_EQ(view1.size(), InitialSize);
	ASSERT_TRUE(checkBufferDefaultContent(view1));

	BufferView view2 = view1; // copy constructor;
	ASSERT_EQ(view2.size(), view1.size());
	ASSERT_EQ(view2.data(), view1.data());

	const size_t view3Offset = view2.size() / 2;

	BufferView view3 {view2, view3Offset};
	ASSERT_EQ(view3.size(), view2.size() - view3Offset);
	ASSERT_TRUE(checkBufferDefaultContent(view3, 0, view3Offset));

	const size_t view4Offset = view3.size() / 2;

	BufferView view4 {view3, view4Offset};
	ASSERT_EQ(view4.size(), view3.size() - view4Offset);
	ASSERT_TRUE(checkBufferDefaultContent(view4, 0, view3Offset + view4Offset));
	ASSERT_EQ(view3.data() + view4Offset, view4.data());

}


TEST(Test_Buffers, InternalStorage)
{
	constexpr size_t InitialSize = 100;

	BytesBuffer buffer(InitialSize);

	fillBufferWithDefaultContent(buffer);

	auto storage = BufferStorage::takeOut(std::move(buffer));

	ASSERT_FALSE(buffer);

	ASSERT_THAT(BufferStorage::size(storage), Eq(InitialSize));

	auto restoredBuffer = BufferStorage::bufferFromStorage(storage);

	ASSERT_THAT(restoredBuffer.size(), Eq(InitialSize));

	ASSERT_THAT(BufferUtils::refsCount(restoredBuffer), Eq(1));

	ASSERT_TRUE(checkBufferDefaultContent(buffer));
}

TEST(Test_Buffers, Concurrent)
{
	constexpr size_t InitialSize = 100;
	constexpr size_t ThreadsCount = 10;

	BytesBuffer buffer(InitialSize);

	ReadOnlyBuffer readBuffer = buffer.toReadOnly();


	std::vector<std::thread> threads;

	for (size_t i = 0; i < ThreadsCount; ++i)
	{
		threads.emplace_back([bufferView = readBuffer]() mutable
		{
			for (size_t x = 0; x < 90; ++x)
			{
				BufferView temp {bufferView, x};
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}
}

namespace {

class TestAllocator : public Allocator
{
	COMCLASS_(Allocator)

public:
	~TestAllocator() {
		LOG_debug_("~TestAllocator()")
	}
private:

	void* realloc(void* ptr, size_t size, std::optional<size_t>) override {
		return ::realloc(ptr, size);
	}

	void free(void* ptr, std::optional<size_t>) override {
		::free(ptr);
	}
};

}

TEST(Test_Buffers, Allocator) {

	ComPtr<Allocator> allocator = com::createInstance<TestAllocator, Allocator>();

	BytesBuffer buffer(100, std::move(allocator));
	fillBufferWithDefaultContent(buffer);

}

