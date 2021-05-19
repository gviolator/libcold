#include "pch.h"
#include "cold/threading/event.h"


namespace cold::threading {



//-----------------------------------------------------------------------------
#pragma region stdlib

#if defined(ATD_THREADING_EVENT_STDLIB)
Event::Event(ResetMode mode, bool signaled): m_event(CreateEvent(nullptr, mode == ResetMode::Manual, signaled ? TRUE : FALSE, nullptr)
	: m_mode{mode}
	, m_state{state}
{}


EventBase::ResetMode EventBase::resetMode() const
{
	return m_mode;
}


void EventBase::set()
{
	std::lock_guard mutualAccess(m_mutex);

	m_state = true;

	if (m_mode == ResetMode::Auto)
	{
		m_signal.notify_one();
	}
	else
	{
		m_signal.notify_all();
	}
}


void EventBase::wait()
{
	std::unique_lock lock(m_mutex);

	m_signal.wait(lock, [this]
	{
		return m_state;
	});

	DEBUG_CHECK(m_state)

	if (m_mode == ResetMode::Auto)
	{
		m_state = false;
	}
}


bool EventBase::wait(std::chrono::milliseconds timeout)
{
	std::unique_lock lock(m_mutex);

	const bool ready = m_signal.wait_for(lock, timeout, [this] { return m_state; });

	if (ready)
	{
		DEBUG_CHECK(m_state)

		if (m_mode == ResetMode::Auto)
		{
			m_state = false;
		}
	}
	else
	{
		DEBUG_CHECK(!m_state)
	}

	return ready;
}


bool EventBase::state() const
{
	return m_state;
}


//-----------------------------------------------------------------------------
AutoResetEvent::AutoResetEvent(bool initialState)
	: EventBase(ResetMode::Auto, initialState)
{}


void AutoResetEvent::reset()
{
	std::lock_guard lock(m_mutex);

	m_state = false;
}


//-----------------------------------------------------------------------------
ManualResetEvent::ManualResetEvent(bool initialState)
	: EventBase(ResetMode::Manual, initialState)
{}


void ManualResetEvent::reset()
{
	std::lock_guard lock(m_mutex);

	m_state = false;
}

#pragma endregion

#elif defined (ATD_THREADING_EVENT_WINAPI)

#pragma region WinAPI

static_assert(sizeof(void*) == sizeof(HANDLE) && std::is_same_v<void*, HANDLE>);

Event::Event(ResetMode mode, bool signaled)
	: m_mode(mode)
	, m_event(CreateEventW(nullptr, mode == ResetMode::Manual, signaled, nullptr))
{}

Event::~Event()
{
	if (m_event){
		CloseHandle(m_event);
	}
}

void Event::set()
{
	SetEvent(m_event);
}

void Event::reset()
{
	ResetEvent(m_event);
}

bool Event::wait(std::optional<std::chrono::milliseconds> timeout)
{
	const DWORD ms = timeout ? static_cast<DWORD>(timeout->count()) : INFINITE;

	const auto waitResult = WaitForSingleObject(m_event, ms);
	if (waitResult == WAIT_OBJECT_0)
	{
		return true;
	}

	if (waitResult != WAIT_TIMEOUT)
	{

	}

	return false;
}

#pragma endregion

#endif


Event::ResetMode Event::resetMode() const
{
	return m_mode;
}

}
