#include "pch.h"
#include "diagbackend.h"
#include "loginternal.h"
#include "runtimecheckinternal.h"

#include "cold/diagnostics/runtimecheck.h"
#include "cold/threading/event.h"
#include "cold/threading/setthreadname.h"



namespace cold::diagnostics {


namespace {

/// <summary>
///
/// </summary>
class UvLoop
{
public:
	UvLoop()
	{
		uv_loop_init(&m_uv);
	}

	~UvLoop()
	{
		uv_loop_close(&m_uv);
	}

	operator uv_loop_t* () {
		return &m_uv;
	}


private:

	uv_loop_t m_uv;
};

/// <summary>
///
/// </summary>
DiagBackend* getOrSetBackendInstance(std::optional<DiagBackend*> newInstance = std::nullopt)
{
	static DiagBackend* instance = nullptr;

	if (newInstance)
	{
		auto prev = instance;
		instance = *newInstance;
		return prev;
	}

	return instance;
}

} // namespace


//-----------------------------------------------------------------------------


DiagBackend::DiagBackend()
{
	threading::Event signal;

	m_thread = std::thread([this, &signal]
	{
		threading::setCurrentThreadName("Diagnostics");

		UvLoop uv;

		uv_async_init(uv, &m_async, [](uv_async_t* handle) noexcept {
			uv_stop(handle->loop);
		});

		uv_async_init(uv, &m_logAsync, [](uv_async_t* handle) noexcept {
			DiagBackend& this_ = *reinterpret_cast<DiagBackend*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(handle)));

			const std::vector<LogEntry> logEntries = [](DiagBackend& this_) {
				std::lock_guard lock {this_.m_logMutex};
				return std::move(this_.m_logEntries);
			}(this_);


			/*for (const auto& [level, context, message] : logEntries){
				LogInternal::instance().write(level, context, message);
			}*/
		});

		uv_handle_set_data(reinterpret_cast<uv_handle_t*>(&m_async), reinterpret_cast<void*>(this));

		uv_handle_set_data(reinterpret_cast<uv_handle_t*>(&m_logAsync), reinterpret_cast<void*>(this));

		signal.set();

		uv_run(uv, UV_RUN_DEFAULT);

		uv_close(reinterpret_cast<uv_handle_t*>(&m_async), nullptr);
		uv_close(reinterpret_cast<uv_handle_t*>(&m_logAsync), nullptr);

		while (uv_loop_alive(uv) != 0 )
		{
			if (uv_run(uv, UV_RUN_ONCE) == 0)
			{
				break;
			}
		}

	});

	signal.wait();

	getOrSetBackendInstance(this);
}


DiagBackend::~DiagBackend()
{
	getOrSetBackendInstance(nullptr);

	if (!m_thread.joinable()) {
		return;
	}

	m_isClosed = true;

	uv_async_send(&m_async);

	m_thread.join();
}


void DiagBackend::queueLog(Log::Level level, LogContext context, std::wstring message)
{
	{
		std::lock_guard lock {m_logMutex};
		m_logEntries.emplace_back(level, std::move(context), std::move(message));
	}

	uv_async_send(&m_logAsync);
}




//-----------------------------------------------------------------------------
//
DiagBackend& backend()
{
	auto instance = getOrSetBackendInstance();
	if (instance == nullptr) {

	}

	return *instance;
}

bool backendExists()
{
	return getOrSetBackendInstance() != nullptr;
}


std::unique_ptr<Backend> Backend::create()
{
	return std::make_unique<DiagBackend>();
}


Backend& Backend::instance()
{
	return backend();
}


bool Backend::exists()
{
	return backendExists();
}


//-----------------------------------------------------------------------------

void Log::write(Log::Level level, LogContext context, std::wstring message)
{
	if (!LogInternal::instance().logLevelApplicable(level)) {
		return;
	}

	if (backendExists()){
		backend().queueLog(level, std::move(context), std::move(message));
		return;
	}

	LogInternal::instance().write(level, context, message);
}


Log::LogHandlerHandle Log::setHandler(LogHandler::UniquePtr handler)
{
	const void* const ptr = LogInternal::instance().setHandler(std::move(handler));

	return ptr;
}


void Log::setLevel(Log::Level level, Log::Level* prevLevel)
{
	if (prevLevel) {
		*prevLevel = LogInternal::instance().level();
	}

	LogInternal::instance().setLevel(level);
}


Log::Level Log::level()
{
	return LogInternal::instance().level();
}

//-----------------------------------------------------------------------------

void RuntimeCheck::setFailureHandler(FailureHandler::UniquePtr handler, FailureHandler::UniquePtr* prev)
{
	RuntimeCheckInternal::instance().setFailureHandler(std::move(handler), prev);
}

void RuntimeCheck::raiseFailure(SourceInfo source, const wchar_t* moduleName, const wchar_t* expression, std::wstring message)
{
	RuntimeCheckInternal::instance().raiseFailure(source, moduleName, expression, std::move(message));
}

}


