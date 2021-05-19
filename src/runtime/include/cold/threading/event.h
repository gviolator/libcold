#pragma once
#include <cold/runtime/runtimeexport.h>

#include <chrono>
#include <optional>


//#define ATD_THREADING_EVENT_FORCE_STDLIB


#if defined(ATD_THREADING_EVENT_FORCE_STDLIB) || !defined(_WIN32)
#define ATD_THREADING_EVENT_STDLIB
#elif defined(_WIN32)
#define ATD_THREADING_EVENT_WINAPI
#endif



#ifdef _MSC_VER

#pragma warning(push)
#pragma warning(disable: 4251) //std class need to have dll-interface

#endif


namespace cold::threading {

/**
*/
class RUNTIME_EXPORT Event
{
public:

	enum class ResetMode {
		Auto,
		Manual
	};


	Event(const Event&) = delete;

	Event(ResetMode mode = ResetMode::Auto, bool signaled = false);

#ifdef ATD_THREADING_EVENT_WINAPI
	~Event();
#endif

	/// <summary>
	/// Get the reset mode of this event. Can be 'Auto' or 'Manual'.
	/// </summary>
	/// <returns>reset mode for current event instance.</returns>
	ResetMode resetMode() const;

	/// <summary>
	/// Sets the state of the event to signaled, allowing one or more waiting threads to proceed.
	/// For an event with reset mode = Auto , the set method releases a single thread. If there are no waiting threads, the wait handle remains signaled until a thread attempts to wait on it, or until its Reset method is called.
	/// </summary>
	void set();

	void reset();

	/// <summary>
	/// Blocks the current thread until the current WaitHandle receives a signal, using the timeout interval in milliseconds.
	/// </summary>
	/// <param name="timeout">timeout</param>
	/// <returns>true if the current event receives a signal; otherwise, false.</returns>
	bool wait(std::optional<std::chrono::milliseconds> timeout = std::nullopt);

	/// <summary>
	/// Blocks the current thread until the current WaitHandle receives a signal, using the timeout interval.
	/// </summary>
	/// <param name="timeout">timeout</param>
	/// <returns>true if the current event receives a signal; otherwise, false.</returns>
	template<typename Rep, typename Period>
	bool wait(std::chrono::duration<Rep, Period> timeout)
	{
		using namespace std::chrono;

		return wait(duration_cast<milliseconds>(timeout));
	}

	///// <summary>
	///// Current state of the event instance.
	///// </summary>
	///// <returns>true if the event in the signaled state; otherwise, false.</returns>
	//bool state() const;

protected:

	ResetMode m_mode;

#ifdef ATD_THREADING_EVENT_STDLIB
	std::mutex m_mutex;
	std::condition_variable m_signal;
	bool m_state;
#elif defined (ATD_THREADING_EVENT_WINAPI)
	void* const m_event;
#endif
};


//
///// <summary>
///// Event with auto reset mode.
///// </summary>
//class RUNTIME_EXPORT AutoResetEvent final : public EventBase
//{
//public:
//
//	AutoResetEvent(bool initialState = false);
//
//	void reset();
//};
//
///// <summary>
///// Event with manual reset mode.
///// </summary>
//class RUNTIME_EXPORT ManualResetEvent final : public EventBase
//{
//public:
//
//	ManualResetEvent(bool initialState = false);
//
//	/// <summary>
//	/// Set the event to non signaled state.
//	/// </summary>
//	void reset();
//};

}


#ifdef _MSC_VER
#pragma warning(pop)
#endif
