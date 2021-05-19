#pragma once
#include <cold/diagnostics/diaginternal.h>
#include <cold/diagnostics/logging.h>
#include <cold/diagnostics/runtimecheck.h>
#include <cold/diagnostics/exception.h>
#include <cold/utils/typeutility.h>
#include <cold/meta/classinfo.h>
#include <cold/compiler/coroutine.h>

#include <array>
#include <exception>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>


namespace cold {

template<typename T = void>
class Result;

struct Success
{};


constexpr inline Success success;

/**
*/
template<typename T = std::exception>
struct InplaceErrorTag
{};

/*
*/
struct InplaceResultTag
{};

/**
*/
template<typename T>
inline const InplaceErrorTag<T> InplaceError;

/**
*/
inline const InplaceResultTag InplaceResult;


/**
*/
struct ExceptionData
{
	static constexpr size_t MaxStoredTypesInfo = 5;

	std::exception_ptr exception;
	size_t typesInfoCount = 0;
	std::array<const std::type_info*, MaxStoredTypesInfo> typesInfo = {nullptr, nullptr, nullptr, nullptr, nullptr};

	ExceptionData(std::exception_ptr exception_): exception(exception_)
	{
		DEBUG_CHECK(exception)
	}

	template<typename E>
	requires (std::is_base_of_v<std::exception, E>)
	ExceptionData(E&& exception): ExceptionData(std::make_exception_ptr(std::move(exception)))
	{
		const auto setExceptionType = [this](const std::type_info& type)
		{
			if (typesInfoCount < typesInfo.size())
			{
				typesInfo[typesInfoCount++] = &type;
			}
		};
		
		const auto initializeExceptionTypes = [&setExceptionType] <typename ... ExcType>(TypeList<ExcType...>)
		{
			(setExceptionType(typeid(ExcType)), ...);
		};

		using Types = typelist::AppendHead<meta::ClassAllUniqueBase<E>, E>;

		initializeExceptionTypes(Types{});
	}

	template<typename E>
	requires (std::is_base_of_v<std::exception, E>)
	bool hasException() const noexcept
	{
		const std::type_info& exceptionType = typeid(E);
		if (exceptionType == typeid(std::exception))
		{
			return true;
		}

		for (size_t i = 0; i < typesInfoCount; ++i)
		{
			if (*typesInfo[i] == exceptionType)
			{
				return true;
			}
		}

		try
		{
			DEBUG_CHECK(this->exception)
			std::rethrow_exception(this->exception);
		}
		catch (const E&)
		{
			return true;
		}
		catch (...)
		{
		}

		return false;
	}
};


namespace internal__ {

template<typename>
struct ResultCoroPromiseBase;

template<typename>
struct ResultCoroPromise;

template<typename>
struct ResultCoroAwaiterRVRef;

template<typename>
struct ResultCoroAwaiterLVRef;

template<typename>
class ResultBase;


/**
*/
template<typename T>
class ResultBase
{
	static_assert(!std::is_same_v<T, std::exception_ptr>, "std::exception_ptr is not acceptable value type for Result<>");
	static_assert(!std::is_same_v<T, Result<T>*>, "Result* is not acceptable value type for Result<>");

protected:
	using ValueType = std::conditional_t<std::is_same_v<void, T>, bool, T>;
	using ResultProxy = Result<T>*;

public:
	using Data = std::variant<ResultProxy, ExceptionData, ValueType>;

	explicit operator bool () const
	{
		return std::holds_alternative<ValueType>(m_data);
	}

	template<typename E, typename ... Args>
	requires std::is_base_of_v<std::exception, E> && std::is_constructible_v<ExceptionImplType<E>, Args...>
	ResultBase(InplaceErrorTag<E>, Args&& ... args): m_data(std::in_place_type<ExceptionData>, ExceptionImplType<E>{std::forward<Args>(args)...} )
	{}

	template<typename E>
	requires (std::is_base_of_v<std::exception, E>)
	ResultBase(E exception): m_data(std::in_place_type<ExceptionData>, std::move(exception))
	{}

	ResultBase(std::exception_ptr exception): m_data(std::in_place_type<ExceptionData>, std::move(exception))
	{}

	ResultBase(ExceptionData exception): m_data(std::in_place_type<ExceptionData>, std::move(exception))
	{}

	ResultBase(ResultBase<T>&& other): m_data(std::move(other.m_data))
	{
		if (Result<T>* const proxy = getProxy(); proxy)
		{
			static_cast<ResultBase<T>*>(proxy)->m_data.template emplace<ResultProxy>(static_cast<Result<T>*>(this));
		}
	}

	ResultBase(const ResultBase& other): m_data(other.data)
	{
		DEBUG_CHECK(!std::holds_alternative<ResultProxy>(m_data))
	}

	ResultBase<T>& operator = (const ResultBase<T>& r)
	{
		m_data = r.m_data;
		DEBUG_CHECK(!std::holds_alternative<ResultProxy>(m_data))
		return *this;
	}

	ResultBase<T>& operator = (ResultBase<T>&& r)
	{
		m_data = std::move(r.m_data);
		DEBUG_CHECK(!std::holds_alternative<ResultProxy>(m_data))
		return *this;
	}

	template<typename E = std::exception>
	requires (std::is_base_of_v<std::exception, E>)
	bool hasException() const noexcept
	{
		const ExceptionData* const exc = std::get_if<ExceptionData>(&m_data);
		return exc && exc->hasException<E>();
	}

	void rethrowIfException() const
	{
		if (const ExceptionData* const exc = std::get_if<ExceptionData>(&m_data); exc)
		{
			DEBUG_CHECK(exc->exception)
			std::rethrow_exception(exc->exception);
		}
	}

	void ignore() const noexcept
	{
		if (this->hasException())
		{
			LOG_warning_("Ignoring Result<{0}> that holds an exception", typeid(T))
		}
	}

	ExceptionData err()
	{
		ExceptionData* const err = std::get_if<ExceptionData>(&m_data);
		RUNTIME_CHECK(err)
		ExceptionData temp = std::move(*err);
		m_data.template emplace<ResultProxy>(nullptr);
		return std::move(temp);
	}

protected:
	~ResultBase()
	{
		if(auto proxy = clearProxy(); proxy)
		{
			proxy->clearProxy();
		}
	}

	ResultBase() = default;

	ResultBase(Result<T>* proxy_): m_data(std::in_place_type<ResultProxy>, proxy_)
	{
		proxy_->m_data.template emplace<ResultProxy>(static_cast<Result<T>*>(this));
	}

	Result<T>* clearProxy()
	{
		if (!std::holds_alternative<ResultProxy>(this->m_data))
		{
			return nullptr;
		}

		Result<T>* proxy = nullptr;
		std::swap(std::get<ResultProxy>(this->m_data), proxy);
		return proxy;
	}

	Result<T>* getProxy() const
	{
		if (!std::holds_alternative<ResultProxy>(this->m_data))
		{
			return nullptr;
		}
		return std::get<ResultProxy>(this->m_data);
	}

	Data m_data;
};


} // namespace internal__




template<typename T>
class [[nodiscard]] Result final : public internal__::ResultBase<T>
{
public:

	using internal__::ResultBase<T>::ResultBase;
	using internal__::ResultBase<T>::operator =;
	using internal__::ResultBase<T>::operator bool;

	template<typename ... Args>
	Result(InplaceResultTag, Args&& ... args)
	{
		this->m_data.template emplace<T>(std::forward<Args>(args)...);
	}

	Result(T&& value) requires std::is_move_constructible_v<T>
	{
		this->m_data.template emplace<T>(std::move(value));
	}

	Result(const T& value) requires std::is_copy_constructible_v<T>
	{
		this->m_data.template emplace<T>(value);
	}

	template<typename U>
	requires (!std::is_same_v<U,T> && std::is_constructible_v<T, U>)
	Result(U&& value)
	{
		this->m_data.template emplace<T>(std::forward<U>(value));
	}

	template<typename U>
	requires (!std::is_same_v<U,T> && std::is_constructible_v<T, U>)
	Result(Result<U>&& other)
	{
		constexpr bool ImplementMe = false;
		static_assert(ImplementMe, "Need to implement");
	}

	template<typename U>
	requires (!std::is_same_v<U,T> && std::is_assignable_v<T&, U>)
	Result& operator = (const Result<U>& other)
	{
		return *this;
	}

	const T& operator* () const &
	{
		this->rethrowIfException();
		return std::get<T>(this->m_data);
	}

	T& operator* () &
	{
		this->rethrowIfException();
		return std::get<T>(this->m_data);
	}

	T&& operator* () &&
	{
		this->rethrowIfException();
		return std::move(std::get<T>(this->m_data));
	}

	const T* operator -> () const
	{
		this->rethrowIfException();
		return &std::get<T>(this->m_data);
	}

	T* operator -> ()
	{
		this->rethrowIfException();
		return &std::get<T>(this->m_data);
	}

private:
	Result() = default;

	Result(Result<T>* proxy_): internal__::ResultBase<T>(proxy_)
	{}

	friend class cold::internal__::ResultCoroPromiseBase<T>;

	friend class cold::internal__::ResultCoroPromise<T>;

	template<typename>
	friend class cold::internal__::ResultCoroAwaiterRVRef;

	template<typename>
	friend class cold::internal__::ResultCoroAwaiterLVRef;
};

/**
*/
template<>
class [[nodiscard]] Result<void> : public internal__::ResultBase<void>
{
public:
	using internal__::ResultBase<void>::ResultBase;
	using internal__::ResultBase<void>::operator =;
	using internal__::ResultBase<void>::operator bool;

	Result(Success)
	{
	 this->m_data.emplace<bool>(true);
	}

private:
	Result() = default;

	friend class cold::internal__::ResultCoroPromiseBase<void>;
	friend class cold::internal__::ResultCoroPromise<void>;

	template<typename>
	friend class cold::internal__::ResultCoroAwaiterRVRef;

	template<typename>
	friend class cold::internal__::ResultCoroAwaiterLVRef;
};

template<typename T>
inline constexpr bool IsResult = cold::IsTemplateOf<Result, T>;

namespace internal__ {

template<typename T>
struct ResultCoroAwaiterRVRef
{
	Result<T> m_result;

	ResultCoroAwaiterRVRef(Result<T>&& result_): m_result(std::move(result_))
	{}

	bool await_ready() const noexcept
	{
		return static_cast<bool>(m_result);
	}

	template<typename U>
	void await_suspend(std::coroutine_handle<ResultCoroPromise<U>> coro_) noexcept;


	T await_resume() noexcept
	{
		return *std::move(m_result);
	}
};


template<typename T>
struct ResultCoroAwaiterLVRef
{
	const Result<T>& m_result;

	ResultCoroAwaiterLVRef(const Result<T>& result_): m_result(result_)
	{}

	bool await_ready() const noexcept
	{
		return static_cast<bool>(m_result);
	}

	template<typename U>
	void await_suspend(std::coroutine_handle<ResultCoroPromise<U>> coro_) noexcept ;

	T await_resume() const noexcept
	{
		return *m_result;
	}
};


template<typename T>
struct ResultCoroPromiseBase
{
	Result<T> result;

	std::suspend_never initial_suspend() noexcept
	{
		return {};
	}

	std::suspend_always final_suspend() noexcept
	{
		return {};
	}

	Result<T> get_return_object()
	{
		return Result<T>{&this->result};
	}

	void unhandled_exception() noexcept
	{
		Result<T>* const proxy = this->result.clearProxy();
		proxy->m_data.template emplace<ExceptionData>(std::current_exception());
	}

	template<typename U>
	static auto await_transform(Result<U>&& result_)
	{
		return ResultCoroAwaiterRVRef<U>{std::move(result_)};
	}

	template<typename U>
	static auto await_transform(const Result<U>& result_)
	{
		return ResultCoroAwaiterLVRef<U>{result_};
	}

	template<typename U>
	static decltype(auto) await_transform(U&& value)
	{
		static_assert(cold::IsTemplateOf<cold::Result ,U>, "Only Result<> can be used for co_await operation");
		return std::forward<U>(value);
	}
};



/**
*/
template<typename T>
struct ResultCoroPromise : ResultCoroPromiseBase<T>
{
	template<typename U>
	void return_value(U&& value)
	{
		static_assert(std::is_constructible_v<T,U>, "Invalid return value type: it can not be converted to coroutine target type");

		Result<T>* const proxy = this->result.clearProxy();
		DEBUG_CHECK(proxy)

		proxy->m_data.template emplace<T>(std::forward<U>(value));
	}
};

template<>
struct ResultCoroPromise<void> : public ResultCoroPromiseBase<void>
{
	void return_void()
	{
		Result<>* const proxy = this->result.clearProxy();
		DEBUG_CHECK(proxy)

		proxy->m_data.template emplace<bool>(true);
	}
};


template<typename T>
template<typename U>
void ResultCoroAwaiterRVRef<T>::await_suspend(std::coroutine_handle<ResultCoroPromise<U>> coro) noexcept
{
	Result<U>* targetResult = coro.promise().result.clearProxy();
	DEBUG_CHECK(targetResult)
	const ExceptionData* const exceptionData = std::get_if<ExceptionData>(&this->m_result.m_data);
	DEBUG_CHECK(exceptionData)
	targetResult->m_data.template emplace<ExceptionData>(*exceptionData);
	coro.destroy();
}


template<typename T>
template<typename U>
void ResultCoroAwaiterLVRef<T>::await_suspend(std::coroutine_handle<ResultCoroPromise<U>> coro) noexcept
{
	Result<U>* targetResult = coro.promise().result.clearProxy();
	DEBUG_CHECK(targetResult)
	ExceptionData* const exceptionData = std::get_if<ExceptionData>(&this->m_result);
	DEBUG_CHECK(exceptionData)
	targetResult->template emplace<ExceptionData>(*exceptionData);
	coro.destroy();
}


} // namespace internal__

} // namespace cold


namespace STD_CORO {

template<typename T, typename ... Args>
struct coroutine_traits<cold::Result<T>, Args...>
{
	using promise_type = cold::internal__::ResultCoroPromise<T>;
};


}