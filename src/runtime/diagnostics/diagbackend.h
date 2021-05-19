#pragma once
#include "cold/diagnostics/backend.h"
#include "cold/diagnostics/logging.h"


namespace cold::diagnostics {

/// <summary>
///
/// </summary>
class DiagBackend : public Backend
{
public:

	DiagBackend();

	~DiagBackend();

	void queueLog(Log::Level level, LogContext context, std::wstring message);

	void close();

private:

	using LogEntry = std::tuple<Log::Level, LogContext, std::wstring>;

	std::thread m_thread;
	uv_async_t m_async;
	uv_async_t m_logAsync;
	bool m_isClosed = false;
	std::mutex m_logMutex;
	std::vector<LogEntry> m_logEntries;
};


bool backendExists();

DiagBackend& backend();

}

