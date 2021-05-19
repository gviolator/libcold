#include "pch.h"
#include "loginternal.h"
#include "diagbackend.h"


namespace cold::diagnostics {

namespace {


/// <summary>
///
/// </summary>
struct ConColor
{
	WORD value;

	static ConColor current(DWORD outputId = STD_OUTPUT_HANDLE)
	{
		const HANDLE outputHandle = GetStdHandle(outputId);

		CONSOLE_SCREEN_BUFFER_INFO info;


		GetConsoleScreenBufferInfo(outputHandle, &info);

		const WORD blue = (info.wAttributes & FOREGROUND_BLUE) != 0 ? FOREGROUND_BLUE : 0;
		const WORD green = (info.wAttributes & FOREGROUND_GREEN) != 0 ? FOREGROUND_GREEN : 0;
		const WORD red = (info.wAttributes & FOREGROUND_RED) != 0 ? FOREGROUND_RED : 0;
		const WORD intensity = (info.wAttributes & FOREGROUND_INTENSITY) != 0 ? FOREGROUND_INTENSITY : 0;

		const auto value = static_cast<WORD>(blue | green | red | intensity);

		return ConColor{value};
	}

	static ConColor red()
	{
		return ConColor{FOREGROUND_RED|FOREGROUND_INTENSITY};
	}

	static ConColor green()
	{
		return ConColor{FOREGROUND_GREEN|FOREGROUND_INTENSITY};
	}

	static ConColor blue()
	{
		return ConColor{FOREGROUND_BLUE|FOREGROUND_INTENSITY};
	}

	static ConColor yellow()
	{
		return ConColor{FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY};
	}


	ConColor(WORD value_): value(value_)
	{}

	void apply(DWORD outputId = STD_OUTPUT_HANDLE) const
	{
		const HANDLE streamHandle = GetStdHandle(outputId);
		SetConsoleTextAttribute(streamHandle, value);
	}
};

struct ConColorGuard
{
	const ConColor keepColor = ConColor::current();

	~ConColorGuard()
	{
		keepColor.apply();
	}
};

}



void LogInternal::write(Log::Level logLevel, const LogContext& context, const std::wstring& message)
{
	if (m_handler)
	{
		m_handler->invoke(logLevel, context, message.c_str());

		return;
	}

	std::wcout << message << std::endl;


#if 0
	//static std::mutex s_logMutex;

	//std::lock_guard<std::mutex> lock(s_logMutex);

	ConColorGuard colorGuard;


	//const bool writeToStdOut = !(level == Log::Level::Debug);

	//if (writeToStdOut)
	//{
	//	std::time_t t = std::time(nullptr);

	//	char mbstr[50];

	//	tm time;

	//	localtime_s(&time, &t);

	//	if (std::strftime(mbstr, sizeof(mbstr), "[%H:%M:%S]", &time))
	//	{
	//		std::cout << mbstr ;
	//	}
	//}

	/*
	auto writeToDebugOutput = [message, &context]
	{
		if (IsDebuggerPresent() == FALSE || message.empty())
		{
			return;
		}

		std::wstring_view src = !context.source.empty() ? context.source : std::wstring_view{L"[nosource]"};

		const auto formattedMessage = format(L"%1(%2): %3\n", src, context.line, message);

		OutputDebugStringW(formattedMessage.c_str());
	};
	*/

	switch (logLevel)
	{
	case Log::Level::Verbose:
	{
		//std::wcout << ConColor{FOREGROUND_GREEN | FOREGROUND_BLUE} << L"V:" << message;

		break;
	}

	case Log::Level::Info:
	{
		//std::wcout << ConColor::blue() << L"I:" << message;

		break;
	}

	case Log::Level::Warning:
	{
		/*std::wcout << ConColor::yellow() << L"W:" << message;

		writeToDebugOutput();*/

		break;
	}

	case Log::Level::Exception:
	case Log::Level::Error:
	{
		std::wcout << ConColor::red() << L"E:" << message;

		writeToDebugOutput();

		break;
	}

	case Log::Level::Debug:
	{
		writeToDebugOutput();

		break;
	}
	}

	//if (writeToStdOut)
	//{
	//	std::wcout << colorGuard.defaultColor << std::endl << L"[";

	//	for (size_t i = 0, count = context.tagsCount(); i < count; ++i)
	//	{
	//		std::cout << ConColor{FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY} << context.tagValue(i);

	//		std::wcout << colorGuard.defaultColor;

	//		if (i != count - 1)
	//		{
	//			std::cout << ';';
	//		}
	//		else
	//		{
	//			std::cout << ']' << std::endl;
	//		}
	//	}

	//	std::wcout << context.function << L" at:" << context.source << L":(" << context.line << L")" << std::endl << std::endl;
	//}
#endif
}


const void* LogInternal::setHandler(Log::LogHandler::UniquePtr handler)
{
	m_handler = std::move(handler);

	return reinterpret_cast<const void*>(m_handler.get());
}


void LogInternal::removeHandler(const void* ptr)
{
	m_handler.reset();
}


void LogInternal::setLevel(Log::Level level)
{
	m_level = level;
}


Log::Level LogInternal::level() const
{
	return m_level;
}


LogInternal& LogInternal::instance()
{
	static LogInternal instance;

	return instance;
}


//-----------------------------------------------------------------------------

Log::LogHandlerHandle::LogHandlerHandle(const void* handle_): m_handle(handle_)
{}

Log::LogHandlerHandle::LogHandlerHandle(LogHandlerHandle&& other): m_handle(other.m_handle)
{
	other.m_handle = nullptr;
}

Log::LogHandlerHandle::~LogHandlerHandle()
{
	if (m_handle){
		LogInternal::instance().removeHandler(m_handle);
	}

}

Log::LogHandlerHandle& Log::LogHandlerHandle::operator = (LogHandlerHandle&& other)
{
	if (m_handle) {
		LogInternal::instance().removeHandler(m_handle);
		m_handle = nullptr;
	}

	std::swap(m_handle, other.m_handle);

	return *this;
}

Log::LogHandlerHandle::operator bool () const noexcept
{
	return m_handle != nullptr;
}

//-----------------------------------------------------------------------------




}
