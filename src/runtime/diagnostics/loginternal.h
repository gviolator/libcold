#pragma once
#include <cold/diagnostics/logging.h>

namespace cold::diagnostics {

/// <summary>
///
/// </summary>
class LogInternal
{
public:

	static LogInternal& instance();


	inline bool logLevelApplicable(Log::Level requestedLevel)
	{
		if (requestedLevel == Log::Level::Critical) {
			return true;
		}

		const auto currentId = static_cast<std::underlying_type_t<Log::Level>>(m_level.load());
		const auto requestedId = static_cast<std::underlying_type_t<Log::Level>>(requestedLevel);

		return requestedId <= currentId;
	}

	void write(Log::Level logLevel, const LogContext& context, const std::wstring& message);

	const void* setHandler(Log::LogHandler::UniquePtr handler);

	void removeHandler(const void* ptr);

	void setLevel(Log::Level);

	Log::Level level() const;

private:

	std::atomic<Log::Level> m_level = Log::Level::Debug;
	Log::LogHandler::UniquePtr m_handler;
	std::mutex m_writeMutex;
};

}
