#include "pch.h"
#include "coretaskimpl.h"

#include "cold/async/core/coretask.h"
#include "cold/async/core/coretasklinkedlist.h"
#include "cold/threading/event.h"
#include "cold/diagnostics/runtimecheck.h"
#include "cold/utils/scopeguard.h"

using namespace cold::async;
using namespace cold::async::core;
using namespace cold::async_internal;


namespace cold { 
namespace async::core {

#pragma region CoreTaskLinkedList

//-----------------------------------------------------------------------------

bool operator == (const CoreTaskLinkedList::iterator& iter1, const CoreTaskLinkedList::iterator& iter2)
{
	return iter1.m_task == iter2.m_task;
}

bool operator != (const CoreTaskLinkedList::iterator& iter1, const CoreTaskLinkedList::iterator& iter2)
{
	return iter1.m_task != iter2.m_task;
}

CoreTaskLinkedList::iterator& CoreTaskLinkedList::iterator::operator++()
{
	DEBUG_CHECK(m_task, "Task iterator is not dereferencable")
	m_task = static_cast<CoreTaskImpl*>(m_task)->next();

	return *this;
}

CoreTaskLinkedList::iterator CoreTaskLinkedList::iterator::operator++(int)
{
	DEBUG_CHECK(m_task, "Task iterator is not dereferencable")
	
	iterator temp{*this};
	this->operator++();
	return temp;
}

CoreTask* CoreTaskLinkedList::iterator::operator*() const
{
	DEBUG_CHECK(m_task, "Task iterator is not dereferencable")
	return m_task;
}

CoreTaskLinkedList::iterator::iterator(CoreTask* task_): m_task(task_)
{
}

//-----------------------------------------------------------------------------
CoreTaskLinkedList::CoreTaskLinkedList(TaskContainerIterator taskIterator, void* iteratorState)
{
	CoreTaskImpl* last = nullptr;

	do
	{
		CoreTaskPtr next = taskIterator(iteratorState);
		if (!next)
		{
			break;
		}

		CoreTaskImpl* const task = static_cast<CoreTaskImpl*>(getCoreTask(next));
		DEBUG_CHECK(task)
		task->addRef();

		if (!m_head)
		{
			m_head = task;
		}

		if (last)
		{
			DEBUG_CHECK(!last->next())
			last->setNext(task);
		}

		last = task;
		++m_size;
	}
	while (true);
}

CoreTaskLinkedList::~CoreTaskLinkedList()
{
	CoreTaskImpl* next = static_cast<CoreTaskImpl*>(m_head);

	while (next)
	{
		CoreTaskImpl* const current = next;
		next = next->next();

		current->setNext(nullptr);
		current->setReadyCallback(nullptr, nullptr);
		current->release();
	}
}

CoreTaskLinkedList::iterator CoreTaskLinkedList::begin()
{
	return {m_head};
}


CoreTaskLinkedList::iterator CoreTaskLinkedList::end()
{
	return iterator {};
}


size_t CoreTaskLinkedList::size() const
{
#ifndef NDEBUG
	const CoreTaskImpl* next = static_cast<const CoreTaskImpl*>(m_head);
	size_t counter = 0;
	while (next)
	{
		++counter;
		next = next->next();
	}

	DEBUG_CHECK(m_size == counter)
#endif

	return m_size;
}


bool CoreTaskLinkedList::empty() const
{
	return m_head == nullptr;
}

#pragma endregion

} // namespace async::core

namespace async_internal {

//-----------------------------------------------------------------------------

bool wait__(CoreTaskPtr taskPtr, std::optional<std::chrono::milliseconds> timeout)
{
	CoreTask* const task = getCoreTask(taskPtr);
	if (task->ready())
	{
		return true;
	}

	SCOPE_Leave {
		task->setReadyCallback(nullptr, nullptr);
	};

	threading::Event signal;
	task->setReadyCallback([](void* ptr, void*) noexcept
	{
		auto& signal = *reinterpret_cast<threading::Event*>(ptr);
		signal.set();
	}, &signal);

	return signal.wait(timeout);
}

Task<bool> whenAll__(TaskContainerIterator taskIterator, void* iteratorState, std::optional<std::chrono::milliseconds> timeout)
{
	CoreTaskLinkedList tasks{taskIterator, iteratorState};

	const size_t size = tasks.size();
	if (size == 0)
	{
		co_return true;
	}

	struct AwaiterState
	{
		std::atomic_size_t counter;
		TaskSource<> taskSource;

	} awaiterState { .counter = size };

	for (CoreTask* const task : tasks)
	{
		task->setReadyCallback([](void* ptr, void*) noexcept
		{
			auto& awaiterState = *reinterpret_cast<AwaiterState*>(ptr);
			if (awaiterState.counter.fetch_sub(1) == 1)
			{
				awaiterState.taskSource.resolve();
			}
		}, &awaiterState);
	}

	co_await awaiterState.taskSource.getTask();

	co_return true;
}

Task<bool> whenAny__(TaskContainerIterator taskIterator, void* iteratorState, std::optional<std::chrono::milliseconds> timeout)
{
	CoreTaskLinkedList tasks{taskIterator, iteratorState};

	if (tasks.empty() || std::any_of(tasks.begin(), tasks.end(), [](const CoreTask* task) {return task->ready(); }))
	{
		co_return true;
	}

	TaskSource<> taskSource;

	for (CoreTask* const task : tasks)
	{
		task->setReadyCallback([](void* ptr, void*) noexcept
		{
			auto& taskSource = *reinterpret_cast<TaskSource<>*>(ptr);
			taskSource.resolve();
		}, &taskSource);
	}

	co_await taskSource.getTask();

	co_return true;
}


} // namespace async_internal
} // namespace cold
