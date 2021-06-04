#include "pch.h"
#include "cold/runtime/runtime.h"
#include "cold/runtime/internal/uvutils.h"
#include "cold/runtime/internal/runtimeinternal.h"
#include "cold/runtime/internal/runtimeallocator.h"

#include "cold/async/task.h"
#include "cold/threading/event.h"
#include "cold/threading/critical_section.h"
#include "cold/utils/intrusivelist.h"

using namespace cold::async;

namespace cold {

namespace {

inline void closeAndFreeUvHandle(uv_handle_t* handle)
{
	DEBUG_CHECK(Runtime::isRuntimeThread())
	DEBUG_CHECK(uv_is_closing(handle) == 0)

	uv_handle_set_data(handle, nullptr);



	if (handle->type == UV_TCP || handle->type == UV_NAMED_PIPE)
	{
		uv_stream_t* const stream = reinterpret_cast<uv_stream_t*>(handle);
		if (uv_is_writable(stream) != 0 && uv_is_active(handle) != 0)
		{
			uv_shutdown_t* const request = reinterpret_cast<uv_shutdown_t*>(runtimePoolAllocate(sizeof(uv_shutdown_t)));

			const auto shutdownCode = uv_shutdown(request, stream, [](uv_shutdown_t* request, int status) noexcept
			{
				uv_handle_t* const handle = reinterpret_cast<uv_handle_t*>(request->handle);

				runtimePoolFree(request);

				uv_close(handle, freeUvHandle);
			});

			if (shutdownCode == 0)
			{
				return;
			}

			runtimePoolFree(request);

		}
	}


	uv_close(handle, freeUvHandle);
}


} // namespace


uv_handle_t* allocateUvHandle(uv_handle_type what) noexcept
{
	DEBUG_CHECK(Runtime::isRuntimeThread())

	const auto handleSize = uv_handle_size(what);
	DEBUG_CHECK(handleSize > 0, "Unknown uv handle ({0}) size", what)

	void* const mem = runtimePoolAllocate(handleSize);
	auto handle = reinterpret_cast<uv_handle_t*>(mem);

#if !defined(NDEBUG) || defined(DEBUG)
	memset(handle, 0, handleSize);
#endif

	return handle;
}

void freeUvHandle(uv_handle_t* handle) noexcept
{
	DEBUG_CHECK(handle)
	runtimePoolFree(handle);
}

std::string uvErrorMessage(int code, std::string_view customMessage) noexcept
{
	if (code == 0)
	{
		return std::string{};
	}

	const char* const errName = uv_err_name(code);
	if (!customMessage.empty())
	{
		return strfmt("%1:%2", errName, customMessage);
	}

	const char* const errStr = uv_strerror(code);
	return strfmt("%1:%2", errName, errStr);
}

uv_handle_type uvHandleType(const std::type_info& type)
{
	if (type == typeid(uv_async_t))
	{
		return UV_ASYNC;
	}
	else if (type == typeid(uv_timer_t))
	{
		return UV_TIMER;
	}
	else if (type == typeid(uv_tcp_t))
	{
		return UV_TCP;
	}
	else if (type == typeid(uv_pipe_t))
	{
		return UV_NAMED_PIPE;
	}
	else if (type == typeid(uv_tty_t))
	{
		return UV_TTY;
	}

	return UV_UNKNOWN_HANDLE;
}


namespace cold_internal {

UvHandleBase::UvHandleBase(uv_handle_type handleType)
{
	if (handleType != UV_UNKNOWN_HANDLE)
	{
		m_handle = allocateUvHandle(handleType);
		DEBUG_CHECK(m_handle)
	}
}

UvHandleBase::~UvHandleBase()
{
	close();
}

void UvHandleBase::close()
{
	if (!m_handle)
	{
		return;
	}

	uv_handle_t* const handle = m_handle;
	m_handle = nullptr;

	if (Runtime::isRuntimeThread())
	{
		closeAndFreeUvHandle(handle);
	}
	else
	{
		auto task = [](uv_handle_t* handle) -> Task<>
		{
			co_await RuntimeInternal::scheduler();
			closeAndFreeUvHandle(handle);

		}(handle);

		task.detach();
	}
}

UvHandleBase::operator bool () const
{
	return m_handle != nullptr;
}

void UvHandleBase::setData(void* data_)
{
	DEBUG_CHECK(m_handle)
	uv_handle_set_data(m_handle, data_);
}

void* UvHandleBase::data() const
{
	DEBUG_CHECK(m_handle)
	return uv_handle_get_data(m_handle);
}

bool UvHandleBase::isAssignable(const std::type_info& type, uv_handle_type handleType)
{
	if (handleType == UV_UNKNOWN_HANDLE)
	{
		return false;
	}

	if (type == typeid(uv_handle_t))
	{
		return true;
	}

	if (type == typeid(uv_stream_t))
	{
		return handleType == UV_TCP || handleType == UV_NAMED_PIPE || handleType == UV_TTY;
	}

	return uvHandleType(type) == handleType;
}

}}
