#pragma once
#include <cold/diagnostics/runtimecheck.h>
#include <cold/threading/lock.h>

#include <mutex>
#include <optional>
#include <synchapi.h> // windows

namespace cold::threading {

/**
*/
class CriticalSection final
{
public:
	CriticalSection(std::optional<uint32_t> spinsCount = std::nullopt)
	{
		if (spinsCount && *spinsCount > 0)
		{
			[[maybe_unused]] const BOOL alwaysSuccess = InitializeCriticalSectionAndSpinCount(&m_mutex, static_cast<DWORD>(*spinsCount));
		}
		else
		{
			InitializeCriticalSection(&m_mutex);
		}
	}

	~CriticalSection()
	{
		DeleteCriticalSection(&m_mutex);
	}

	CriticalSection(const CriticalSection&) = delete;
	CriticalSection operator = (const CriticalSection&) = delete;

	void lock()
	{
		EnterCriticalSection(&m_mutex);
	}

	void unlock()
	{
		LeaveCriticalSection(&m_mutex);
	}

private:
	CRITICAL_SECTION m_mutex;
};

}

namespace std {

template<>
class lock_guard<cold::threading::CriticalSection> final
{
public:
	lock_guard(cold::threading::CriticalSection& mutex_): m_mutex(mutex_) {
		m_mutex.lock();
	}

	~lock_guard() {
		m_mutex.unlock();
	}

	lock_guard(const lock_guard&) = delete;
	lock_guard operator = (const lock_guard&) = delete;

private:

	cold::threading::CriticalSection& m_mutex;
};

}
