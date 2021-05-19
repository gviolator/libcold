#pragma once

#include <cold/runtime/runtimeexport.h>
#include <cold/memory/allocator.h>
#include <cold/com/ianything.h>
#include <cold/com/comptr.h>

#include <limits>
#include <optional>
#include <string_view>


namespace cold {

class BufferBase;
class BytesBuffer;
class ReadOnlyBuffer;
class BufferView;
struct BufferStorage;
struct BufferUtils;

std::string_view asStringView(const BufferBase& buffer);


/**
*/
class RUNTIME_EXPORT BufferBase
{
public:
	struct Header;

	size_t size() const;

	bool empty() const;

	explicit operator bool () const;

	void release();

	bool sameBufferObject(const BufferBase&) const;

	bool sameBufferObject(const BufferView&) const;

protected:
	BufferBase();
	explicit BufferBase(std::byte*);
	~BufferBase();
	Header& header();
	const Header& header() const;

	std::byte* m_storage;

	friend struct BufferStorage;
	friend struct BufferUtils;

	friend std::string_view asStringView(const BufferBase& buffer);
};


/**
*/
class RUNTIME_EXPORT BytesBuffer : public BufferBase
{
public:
	static constexpr size_t UnspecifiedSize = std::numeric_limits<size_t>::max();

	/**
	*/
	BytesBuffer();

	BytesBuffer(size_t size, ComPtr<Allocator> = {});

	BytesBuffer(const BytesBuffer& buffer) = delete;

	/**
	*/
	BytesBuffer(BytesBuffer&& buffer) noexcept;

	BytesBuffer(BufferView&& buffer) noexcept;

	BytesBuffer& operator = (const BytesBuffer& buffer) = delete;

	/**
	*/
	BytesBuffer& operator = (BytesBuffer&& buffer)  noexcept;

	BytesBuffer& operator = (BufferView&& buffer) noexcept;

	void operator += (BufferView&&) noexcept;

	/**
	*/
	std::byte* data() const;

	/**
	*/
	std::byte* append(size_t size);

	/**
	*/
	void resize(size_t);

	/**
	*/
	ReadOnlyBuffer toReadOnly();

private:
	using BufferBase::BufferBase;

	friend class ReadOnlyBuffer;
	friend class BufferStorage;

};

/**
*/
class RUNTIME_EXPORT ReadOnlyBuffer : public BufferBase
{
public:
	ReadOnlyBuffer();
	explicit ReadOnlyBuffer(BytesBuffer&&) noexcept;
	ReadOnlyBuffer(const ReadOnlyBuffer&);
	ReadOnlyBuffer(ReadOnlyBuffer&&) noexcept;

	ReadOnlyBuffer& operator = (BytesBuffer&&) noexcept;
	ReadOnlyBuffer& operator = (const ReadOnlyBuffer&);
	ReadOnlyBuffer& operator = (ReadOnlyBuffer&&) noexcept;
	
	const std::byte* data() const;

	BytesBuffer toBuffer();

	friend class BytesBuffer;
};

/**

*/
class RUNTIME_EXPORT BufferView
{
public:
	BufferView();
	BufferView(BytesBuffer&&) noexcept;
	BufferView(const ReadOnlyBuffer&, size_t offset_ = 0, std::optional<size_t> size_ = std::nullopt);
	BufferView(ReadOnlyBuffer&&, size_t offset_ = 0, std::optional<size_t> size_ = std::nullopt);
	BufferView(const BufferView&, size_t offset_, std::optional<size_t> size_ = std::nullopt);
	BufferView(BufferView&&, size_t offset_, std::optional<size_t> size_ = std::nullopt);
	BufferView(const BufferView&);
	BufferView(BufferView&&) noexcept;

	BufferView& operator = (const BufferView&);

	BufferView& operator = (BufferView&&) noexcept;

	bool operator == (const BufferView&) const;

	bool operator != (const BufferView&) const;

	void release();

	const std::byte* data() const;

	size_t size() const;

	size_t offset() const;

	explicit operator bool () const;


	/**
		Get internally referenced buffer
	*/
	ReadOnlyBuffer underlyingBuffer() const;

	BytesBuffer toBuffer();

	/**
		Merge this buffer with another one. This method does not modify buffer itself.
		Internally BufferView::merge used to produce result.
	*/
	// BufferView merge(const BufferView& buffer) const;


	/// <summary>
	/// Merge buffers into the new one. If both buffers points to the same (internal) buffer a copy will not be created (only internal parameters will be adjusted).
	/// </summary>
	/// <param name="buffer1">First buffer to merge</param>
	/// <param name="buffer2">Second buffer to merge</param>
	/// <returns>Merged buffer.</returns>
	// static BufferView merge(const BufferView& buffer1, const BufferView& buffer2);

private:
	ReadOnlyBuffer m_buffer;
	size_t m_offset;
	size_t m_size;

	friend struct BufferUtils;
	friend class BytesBuffer;
};



/**
*/
struct BufferStorage
{
	/**
	*/
	RUNTIME_EXPORT static std::byte* allocate(size_t size, ComPtr<Allocator> = {});

	/**
	*/
	RUNTIME_EXPORT static void reallocate(std::byte*& storage, size_t size);

	/**
	*/
	RUNTIME_EXPORT static void release(std::byte*& storage);

	/**
	*/
	RUNTIME_EXPORT static std::byte* takeOut(BufferBase&&);

	/**
	*/
	RUNTIME_EXPORT static std::byte* data(std::byte* storage);

	RUNTIME_EXPORT static size_t size(const std::byte* storage);

	RUNTIME_EXPORT static BytesBuffer bufferFromStorage(std::byte* storage);

	RUNTIME_EXPORT static BytesBuffer bufferFromClientData(std::byte* ptr, std::optional<size_t> size = std::nullopt);
};

/**
*/
struct BufferUtils
{
	RUNTIME_EXPORT static uint32_t refsCount(const BufferBase&);
	RUNTIME_EXPORT static uint32_t refsCount(const BufferView&);
	RUNTIME_EXPORT static BytesBuffer copy(const BufferBase&, size_t offset = 0, std::optional<size_t> size_ = std::nullopt);
	RUNTIME_EXPORT static BytesBuffer copy(const BufferView&, size_t offset = 0, std::optional<size_t> size_ = std::nullopt);
};


inline std::string_view asStringView(const BufferBase& buffer)
{
	if (!buffer || buffer.size() == 0)
	{
		return std::string_view{};
	}
	const std::byte* const data = BufferStorage::data(buffer.m_storage);
	return std::string_view{reinterpret_cast<const char*>(data), buffer.size()};
}

}
