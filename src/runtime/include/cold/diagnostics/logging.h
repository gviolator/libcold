#pragma once

#include <cold/runtime/runtimeexport.h>
#include <cold/diagnostics/sourceinfo.h>
#include <cold/diagnostics/diaginternal.h>
#include <cold/utils/functor.h>


#include <array>
#include <chrono>

namespace cold::diagnostics {


struct LogTagsNode
{
	const wchar_t* const* tags;
	size_t count;
};


namespace diagnostics_internal {



template<size_t Count>
struct LogTagsHolder : LogTagsNode {

	const std::array<const wchar_t*, Count> tagsArray;

	template<typename ... T>
	constexpr LogTagsHolder(T ... args): tagsArray { args ... }
	{
		tags = tagsArray.data();
		count = tagsArray.size();
	}
};


template<typename ... T>
LogTagsHolder(T ... args) -> LogTagsHolder<sizeof ...(T)>;


namespace {

/*constinit */const LogTagsNode* scopeDefaultLogTags__ = nullptr;


/// <summary>
///
/// </summary>
struct SetScopeDefaultLogTags__
{
	SetScopeDefaultLogTags__(const LogTagsNode& tags) {
		scopeDefaultLogTags__ = &tags;
	}
};

} // namespace




} // namespace diagnostics_internal


/// <summary>
///
/// </summary>
struct LogContext
{
	struct LogTags
	{
		static constexpr size_t TagNodesCount = 1;

		std::array<const LogTagsNode*, TagNodesCount> nodes;

		LogTags() = default;

		LogTags(const LogTagsNode* tags_): nodes {tags_}
		{}
	};

	using TimeStamp = decltype(std::chrono::system_clock::now());

	TimeStamp timeStamp = std::chrono::system_clock::now();
	SourceInfo source;
	LogTags tags;

	LogContext(SourceInfo source_): source(source_)
	{}

	LogContext(SourceInfo source_, const LogTagsNode* tags_): source(source_), tags{tags_}
	{}
};


/// <summary>
///
/// </summary>
struct Log
{
	/// <summary>
	///
	/// </summary>
	enum class Level {
		Critical,
		Error,
		Warning,
		Info,
		Debug,
		Verbose
	};


	using LogHandler = Functor<void (Level, const LogContext&, const wchar_t* message) noexcept>;

	class RUNTIME_EXPORT [[nodiscard]] LogHandlerHandle
	{
	public:

		LogHandlerHandle() = default;

		LogHandlerHandle(const LogHandlerHandle&) = delete;

		LogHandlerHandle(LogHandlerHandle&&);

		~LogHandlerHandle();

		LogHandlerHandle& operator = (LogHandlerHandle&&);

		explicit operator bool () const noexcept;

	private:

		LogHandlerHandle(const void* handle);

		const void* m_handle = nullptr;

		friend struct Log;
	};



	RUNTIME_EXPORT static void write(Log::Level, LogContext, std::wstring message);

	RUNTIME_EXPORT static LogHandlerHandle setHandler(LogHandler::UniquePtr);

	RUNTIME_EXPORT static void setLevel(Log::Level, Log::Level* prevLevel = nullptr);

	RUNTIME_EXPORT static Log::Level level();
};

}



#define LOG_TAGS_VNAME(name) CONCATENATE(CONCATENATE(logTags_,name),__)

#define DEFINE_LOG_TAGS(name, ...) \
	constinit auto LOG_TAGS_VNAME(name) = cold::diagnostics::diagnostics_internal::LogTagsHolder{__VA_ARGS__};


#define LOG_TAGS(name, ...) \
namespace {\
DEFINE_LOG_TAGS(name, __VA_ARGS__)\
}\


#define SCOPE_DEFAULT_LOG_TAGS(tags)\
namespace {\
const ::cold::diagnostics::diagnostics_internal::SetScopeDefaultLogTags__ setScopeLogTags__{LOG_TAGS_VNAME(tags)}; \
}\

#define SCOPE_DEFAULT_LOG_TAGS_PTR ::cold::diagnostics::diagnostics_internal::scopeDefaultLogTags__

#define LOG_CONTEXT_WITH_TAGS(tags) ::cold::diagnostics::LogContext{INLINED_SOURCE_INFO, &LOG_TAGS_VNAME(tags)}

#define LOG_CONTEXT ::cold::diagnostics::LogContext{INLINED_SOURCE_INFO, SCOPE_DEFAULT_LOG_TAGS_PTR}


#define LOG_write(level, tags, message, ...) ::cold::diagnostics::Log::write(level, LOG_CONTEXT_WITH_TAGS(tags), cold::diagnostics_internal::diagWStringMessage(message, __VA_ARGS__));

#define LOG_write_(level, message, ...) ::cold::diagnostics::Log::write(level, LOG_CONTEXT, cold::diagnostics_internal::diagWStringMessage(message, __VA_ARGS__));

#define LOG_verbose(tags, message, ...) LOG_write(::cold::diagnostics::Log::Level::Verbose, tags, message, __VA_ARGS__)

#define LOG_verbose_(message, ...) LOG_write_(::cold::diagnostics::Log::Level::Verbose, message, __VA_ARGS__)

#define LOG_debug(tags, message, ...) LOG_write(::cold::diagnostics::Log::Level::Debug, tags, message, __VA_ARGS__)

#define LOG_debug_(message, ...) LOG_write_(::cold::diagnostics::Log::Level::Debug, message, __VA_ARGS__)

#define LOG_info(tags, message, ...) LOG_write(::cold::diagnostics::Log::Level::Info, tags, message, __VA_ARGS__)

#define LOG_info_(message, ...) LOG_write_(::cold::diagnostics::Log::Level::Info, message, __VA_ARGS__)

#define LOG_warning(tags, message, ...) LOG_write(::cold::diagnostics::Log::Level::Warning, tags, message, __VA_ARGS__)

#define LOG_warning_(message, ...) LOG_write_(::cold::diagnostics::Log::Level::Warning, message, __VA_ARGS__)

#define LOG_error(tags, message, ...) LOG_write(::cold::diagnostics::Log::Level::Error, tags, message, __VA_ARGS__)

#define LOG_error_(message, ...) LOG_write_(::cold::diagnostics::Log::Level::Error, message, __VA_ARGS__)

#define LOG_critical(tags, message, ...) LOG_write(::cold::diagnostics::Log::Level::Error, tags, message, __VA_ARGS__)

#define LOG_critical_(message, ...) LOG_write_(::cold::diagnostics::Log::Level::Error, message, __VA_ARGS__)
