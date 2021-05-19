#include "pch.h"
#include "uvstream.h"
#include "cold/network/networkexception.h"
#include "cold/runtime/runtime.h"
#include "cold/runtime/internal/runtimeinternal.h"
#include "cold/diagnostics/exceptionguard.h"
#include "cold/com/comclass.h"

using namespace cold::async;
using namespace cold::atd_literals;

namespace cold::network {

namespace {

constexpr size_t MaxInboundBufferSize = 2048_b;
constexpr size_t MinInboundBufferSize = 128_b;
constexpr size_t InboundBufferGranularity = 64_b;
constexpr size_t DefaultInboundBufferSize = MaxInboundBufferSize;


inline size_t inboundBufferSize(Byte size)
{
	if (size == 0)
	{
		return DefaultInboundBufferSize;
	}

	if (size < MinInboundBufferSize)
	{
		return MinInboundBufferSize;
	}
	else if (size >= MaxInboundBufferSize)
	{
		return MaxInboundBufferSize;
	}

	return alignedSize(size, InboundBufferGranularity);
}

} // namespace



class UvStreamImpl final : public UvStream
{
	COMCLASS_(UvStream)

public:

	UvStreamImpl(UvStreamHandle&& stream): UvStream(std::move(stream))// , m_networkItem(*this)
	{}

	~UvStreamImpl()
	{
		// std::this_thread::sleep_for(std::chrono::milliseconds(3));
	}

	// const NetworkItem m_networkItem;
};

//-----------------------------------------------------------------------------
UvStream::UvStream()
{}


UvStream::UvStream(UvStreamHandle&& stream)
{
	this->setStream(std::move(stream));
}


UvStream::~UvStream()
{
	this->doDispose(true);
}


void UvStream::dispose()
{
	this->doDispose(false);
}


void UvStream::doDispose(bool destructed) noexcept
{
	if (const bool alreadyDisposed = m_isDisposed.exchange(true); alreadyDisposed)
	{
		DEBUG_CHECK(!destructed || !m_stream)
		return;
	}

	auto closeStream = [](UvStream& this_) noexcept
	{
		DEBUG_CHECK(Runtime::isRuntimeThread())

		if (!this_.m_stream)
		{
			return;
		}

		DEBUG_NOEXCEPT_Guard
		{
			if (this_.m_stream && uv_is_active(this_.m_stream) != 0)
			{
				[[maybe_unused]] const auto code = uv_read_stop(this_.m_stream);
			}

			this_.m_stream.close();
			this_.notifyReadAwaiter();
		};
	};

	if (Runtime::isRuntimeThread())
	{
		closeStream(*this);
		return;
	}

	if (destructed)
	{
		auto task = [](UvStream& this_, auto closeStream) -> Task<>
		{
			co_await RuntimeInternal::scheduler();
			closeStream(this_);
		}
		(*this, closeStream);

		async::wait(task);
	}
	else
	{
		[](UvStream& this_, auto closeStream) -> Task<>
		{
			KeepRC(this_);
			co_await RuntimeInternal::scheduler();
			closeStream(this_);
		}
		(*this, closeStream).detach();
	}
}


void UvStream::setStream(UvStreamHandle&& stream)
{
	DEBUG_CHECK(!m_stream)

	m_stream = std::move(stream);

	DEBUG_CHECK(m_stream)

	m_stream->data = this;
}


UvStreamHandle& UvStream::stream()
{
	DEBUG_CHECK(m_stream, "Stream not initialized")
	return (m_stream);
}


Task<> UvStream::write(BufferView buffer)
{ // buffer will be referenced during call and will be automatically released after buffer leave its scope.
	KeepRC(*this);

	SWITCH_RUNTIME_SCHEDULER
	THROW_IF_DISPOSED(*this)
	if (!m_stream || uv_is_writable(m_stream) == 0)
	{
		// LOG_warning_("Attempt to write, but stream is closed")
		co_return;
	}

	const auto writeCallback = [](uv_write_t* request, int status)
	{
		auto& promise = *reinterpret_cast<TaskSource<>*>(request->data);

		if (status != 0)
		{
			//auto error = format("Write operation failed: [%1]", UvUtils::errorMessage(status));
			promise.reject(Excpt(NetworkException, uvErrorMessage(status)));
		}
		else
		{
			promise.resolve();
		}
	};

	uv_buf_t uvBuffer;
	uvBuffer.base = reinterpret_cast<char*>(const_cast<std::byte*>(buffer.data()));
	uvBuffer.len = static_cast<ULONG>(buffer.size());

	TaskSource<> promise;
	uv_write_t op;
	op.data = &promise;

	if (const auto result = uv_write(&op, m_stream, &uvBuffer, 1, writeCallback); result != 0)
	{
		co_return ;
	}

	co_await promise.getTask();
}


Task<BytesBuffer> UvStream::read()
{
	KeepRC(*this);

	SWITCH_RUNTIME_SCHEDULER

	if (m_isDisposed.load() || !m_stream)
	{
		if (!m_buffer)
		{
			LOG_warning_("Attempt to read, but stream is closed")
		}

		co_return std::move(m_buffer);
	}

	DEBUG_CHECK(!m_readAwaiter, "::read operation already processed")

	if (uv_is_active(m_stream) == 0)
	{
		const auto allocateCallback = [](uv_handle_t* handle, size_t size, uv_buf_t* buffer) noexcept
		{
			NOEXCEPT_Guard
			{
				auto& this_ = *reinterpret_cast<UvStream*>(handle->data);

				std::byte* const storage= BufferStorage::allocate(inboundBufferSize(this_.m_inboundBufferSize));

				buffer->base = reinterpret_cast<char*>(BufferStorage::data(storage));
				buffer->len = static_cast<decltype(uv_buf_t::len)>(BufferStorage::size(storage));
			};
		};

		const auto readCallback = [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* inboundBuffer) noexcept
		{
			DEBUG_CHECK(stream)
			if (!stream->data)
			{
				return;
			}

			// LOG_debug_("read ({0})", reinterpret_cast<void*>(stream));

			DEBUG_NOEXCEPT_Guard
			{
				auto& this_ = *reinterpret_cast<UvStream*>(stream->data);

				if (nread < 0)
				{
					if (const int reason = static_cast<int>(nread); reason != UV__EOF)
					{
						// [[maybe_unused]] const char* message = uvErrorMessage(reason); //uv_err_name(static_cast<int>(nread));
						LOG_debug_("UV Stream stream read error: (%1)", uvErrorMessage(reason))
					}

					this_.m_stream.close();
				}
				else
				{
					BytesBuffer buffer = BufferStorage::bufferFromClientData(reinterpret_cast<std::byte*>(inboundBuffer->base), static_cast<size_t>(nread));
					this_.m_buffer += std::move(buffer);
				}

				this_.notifyReadAwaiter();
			}; // NOEXCEPT
		};

		if (const auto code = uv_read_start(m_stream, allocateCallback, readCallback); code != 0)
		{
			if (code == UV__ENOTCONN)
			{
				co_return BytesBuffer{};
			}
		}
	}

	if (m_buffer.size() == 0)
	{
		m_readAwaiter = {};
		co_await m_readAwaiter.getTask();
	}

	co_return std::move(m_buffer);
}


bool UvStream::isDisposed() const
{
	return m_isDisposed;
}


void UvStream::notifyReadAwaiter()
{
	DEBUG_CHECK(Runtime::isRuntimeThread())
	if (m_readAwaiter)
	{
		TaskSource<> awaiter = std::move(m_readAwaiter);
		awaiter.resolve();
	}

	DEBUG_CHECK(!m_readAwaiter)
}


size_t UvStream::setInboundBufferSize(size_t size)
{
	const size_t oldSizeValue = m_inboundBufferSize;
	m_inboundBufferSize = size == 0 ? size : inboundBufferSize(size);

	return m_inboundBufferSize;
}


Stream::Ptr UvStream::create(UvStreamHandle&& stream)
{
	return com::createInstance<UvStreamImpl, Stream>(std::move(stream));
}

}
