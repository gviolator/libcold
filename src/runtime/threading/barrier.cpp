#include "pch.h"
#include "cold/threading/barrier.h"

namespace cold::threading {

Barrier::Barrier(size_t total): m_total(total)
{
	//DEBUG_CHECK(m_total > 0)
}


void Barrier::enter()
{
	const size_t maxExpectedCounter = m_total - 1;

	const size_t counter = m_counter.fetch_add(1);

	//DEBUG_CHECK(counter < m_total)

	if (counter == maxExpectedCounter)
	{
		m_signal.notify_all();

		return;
	}

	std::unique_lock lock(m_mutex);

	m_signal.wait(lock, [this]
	{
		return m_counter.load() == m_total;
	});
}

}
