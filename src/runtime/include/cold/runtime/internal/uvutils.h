#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/runtime/runtime.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/utils/preprocessor.h>
#include <cold/utils/typeutility.h>
#include <cold/network/networkexception.h>

#include <uv.h>
#include <string>
#include <type_traits>

namespace cold {

template<typename>
class UvHandle;

namespace cold_internal {

class RUNTIME_EXPORT UvHandleBase
{
public:

	UvHandleBase() = default;

	UvHandleBase(uv_handle_type);

	UvHandleBase(const UvHandleBase&) = delete;

	~UvHandleBase();

	void close();

	explicit operator bool () const;

	void setData(void*);

	void* data() const;

protected:

	static bool isAssignable(const std::type_info& type, uv_handle_type handleType);


	uv_handle_t* m_handle = nullptr;

	template<typename>
	friend class UvHandle;

};

} // namespace cold_internal

RUNTIME_EXPORT uv_handle_t* allocateUvHandle(uv_handle_type what) noexcept;

RUNTIME_EXPORT void freeUvHandle(uv_handle_t*) noexcept;

RUNTIME_EXPORT std::string uvErrorMessage(int code, std::string_view customMessage = {}) noexcept;

RUNTIME_EXPORT uv_handle_type uvHandleType(const std::type_info& handleType);

template<typename T>
inline uv_handle_t* asUvHandle(T* handle)
{
	static_assert(sizeof(T) >= sizeof(uv_handle_t));
	DEBUG_CHECK(handle)
	
	return reinterpret_cast<uv_handle_t*>(handle);
}
/**
*/
template<typename T = uv_handle_t>
class UvHandle : public cold_internal::UvHandleBase
{
	template<typename U>
	static constexpr bool CrossAssignable = !std::is_same_v<U,T> && (AnyOf<T, uv_handle_t, uv_stream_t> || AnyOf<U, uv_handle_t, uv_stream_t>);

public:

	UvHandle(): cold_internal::UvHandleBase(uvHandleType(typeid(T)))
	{}

	UvHandle(UvHandle&& other)
	{
		std::swap(m_handle, other.m_handle);
		DEBUG_CHECK(!other.m_handle)
	}

	template<typename U>
	UvHandle(UvHandle<U>&& other) requires CrossAssignable<U>
	{
		if (other.m_handle)
		{
			uv_handle_t* const otherHandle = static_cast<UvHandleBase&>(other).m_handle;

			DEBUG_CHECK(UvHandleBase::isAssignable(typeid(T), otherHandle->type))
			m_handle = asUvHandle(other.m_handle);
			other.m_handle = nullptr;
		}
	}

	UvHandle<T>& operator = (UvHandle&& other)
	{
		close();
		DEBUG_CHECK(!m_handle)
		std::swap(m_handle, other.m_handle);

		return *this;
	}

	template<typename U>
	UvHandle<T>& operator = (UvHandle<U>&& other) requires CrossAssignable<U>
	{
		close();
		if (other.m_handle)
		{
			DEBUG_CHECK(UvHandleBase::isAssignable(typeid(T), other.m_handle->type))
			m_handle = asUvHandle(other.m_handle);
			other.m_handle = nullptr;
		}

		return *this;
	}

	T* operator -> () const
	{
		DEBUG_CHECK(m_handle)
		DEBUG_CHECK(m_handle->type == UV_ASYNC || Runtime::isRuntimeThread())

		return reinterpret_cast<T*>(m_handle);
	}

	operator T* () const requires (!std::is_same_v<T, uv_handle_t>)
	{
		DEBUG_CHECK(m_handle)
		DEBUG_CHECK(m_handle->type == UV_ASYNC || Runtime::isRuntimeThread())

		return reinterpret_cast<T*>(m_handle);
	}

	operator uv_handle_t* () const
	{
		DEBUG_CHECK(m_handle)
		DEBUG_CHECK(m_handle->type == UV_ASYNC || Runtime::isRuntimeThread())

		return m_handle;
	}

	template<typename U>
	U* as() const
	{
		DEBUG_CHECK(m_handle)
		DEBUG_CHECK(UvHandleBase::isAssignable(typeid(U), m_handle->type))

		return reinterpret_cast<U*>(m_handle);
	}

	template<typename> friend 
	class UvHandle;
};

using UvStreamHandle = UvHandle<uv_stream_t>;

} // namespace cold


#define UV_RUNTIME_CHECK(expression)\
	if (const int resultCode__ = expression; resultCode__ != 0) \
	{ \
		const auto customMessage__ = cold::uvErrorMessage(resultCode__);\
		cold::diagnostics::RuntimeCheck::raiseFailure(INLINED_SOURCE_INFO, nullptr, L ## #expression, cold::strings::wstringFromUtf8(customMessage__)); \
	}\

#define UV_THROW_ON_ERROR(expression)\
	if (const int resultCode__ = expression; resultCode__ != 0) \
	{ \
		const auto customMessage__ = cold::uvErrorMessage(resultCode__);\
		throw Excpt(cold::network::NetworkException, customMessage__ );\
	}\

