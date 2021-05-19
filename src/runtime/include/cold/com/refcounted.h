#pragma once
#include <cold/diagnostics/runtimecheck.h>
#include <cold/meta/concepts.h>

#include <atomic>
#include <thread>

namespace cold {

//////////////namespace interal__ {

// #ifdef _MSC_VER
// #if defined(_THREAD_)

// using ThreadId = std::thread::id;

// inline auto currentThreadId()
// {
// 	return std::this_thread::get_id();
// }

// #elif defined _WINNT_

// using ThreadId = DWORD;

// inline auto currentThreadId()
// {
// 	return GetCurrentThreadId();
// }

// #else

//}


/*!
*/
template<typename T>
concept RefCounted = requires(T rc)
{
	{ rc.addRef() } -> concepts::Same<uint32_t>;
	{ rc.removeRef() } -> concepts::Same<uint32_t>;
	{ rc.noRefs() } -> concepts::Same<bool>;
};


/*!

*/
class ConcurrentRC
{
public:
	uint32_t addRef()
	{
		const auto prev = m_counter.fetch_add(1);
		DEBUG_CHECK(prev > 0)

		return prev;
	}

	uint32_t removeRef()
	{
		const auto prev = m_counter.fetch_sub(1);
		DEBUG_CHECK(prev > 0)

		return prev;
	}

	bool noRefs() const
	{
		return m_counter.load() == 0;
	}

	bool tryAddRef()
	{
		uint32_t counter = m_counter.load();

		do
		{
			if (counter == 0)
			{
				return false;
			}
		}
		while (!m_counter.compare_exchange_weak(counter, counter + 1));

		return true;
	}

private:
	std::atomic_uint32_t m_counter{1ui32};

	friend inline uint32_t refsCount(const ConcurrentRC& rc_)
	{
		return rc_.m_counter.load();
	}
};


/*!

*/
class SingleThreadRC
{
public:

	uint32_t addRef()
	{
		DEBUG_CHECK(m_counter > 0)
		return m_counter++;
	}

	uint32_t removeRef()
	{
		DEBUG_CHECK(m_counter > 0)
		return m_counter--;
	}

	bool noRefs() const
	{
		return m_counter == 0;
	}


private:
	uint32_t m_counter = 1ui32;

	friend inline uint32_t refsCount(const SingleThreadRC& rc_)
	{
		return rc_.m_counter;
	}
};


/*!
*/
class StrictSingleThreadRC
{
public:
	uint32_t addRef()
	{
		DEBUG_CHECK(m_ownerThreadId == std::this_thread::get_id())
		DEBUG_CHECK(m_counter > 0)

		return m_counter++;
	}

	uint32_t removeRef()
	{
		DEBUG_CHECK(m_ownerThreadId == std::this_thread::get_id())
		DEBUG_CHECK(m_counter > 0)
		
		return m_counter--;
	}

	bool noRefs() const
	{
		DEBUG_CHECK(m_ownerThreadId == std::this_thread::get_id())
		return m_counter == 0;
	}

private:
	uint32_t m_counter = 1ui32;
	const std::thread::id m_ownerThreadId = std::this_thread::get_id();

	friend inline uint32_t refsCount(const StrictSingleThreadRC& rc_)
	{
		DEBUG_CHECK(rc_.m_ownerThreadId == std::this_thread::get_id())
		return rc_.m_counter;
	}

};

/*!

*/
template<RefCounted RC>
inline bool tryAddRef(RC& refCounted)
{
	if (refCounted.noRefs())
	{
		return false;
	}

	refCounted.addRef();
	return true;
}

/*!
*/
inline bool tryAddRef(ConcurrentRC& refCounted)
{
	return refCounted.tryAddRef();
}

}
