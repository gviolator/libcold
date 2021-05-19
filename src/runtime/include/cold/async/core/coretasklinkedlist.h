#pragma once
#include "coretask.h"
#include <type_traits>

namespace cold::async::core {


using TaskContainerIterator = CoreTaskPtr (*)(void*) noexcept;

/**
*/
class CoreTaskLinkedList
{
public:

	class iterator
	{
	public:
		using iterator_category = std::input_iterator_tag;
		using value_type = CoreTask*;
		using difference_type = ptrdiff_t;
		using pointer = std::add_pointer_t<value_type>;
		using reference = std::add_lvalue_reference<value_type>;

		iterator& operator++();
		iterator operator++(int);
		CoreTask* operator*() const;

	private:
		iterator(CoreTask* = nullptr);

		CoreTask* m_task;

		friend bool operator == (const iterator&, const iterator&);
		friend bool operator != (const iterator&, const iterator&);
		friend class CoreTaskLinkedList;
	};

	CoreTaskLinkedList(TaskContainerIterator, void* data_);
	~CoreTaskLinkedList();
	iterator begin();
	iterator end();
	size_t size() const;
	bool empty() const;

private:
	CoreTask* m_head = nullptr;
	size_t m_size = 0;
};


/**
*/
template<typename Container>
struct TaskContainerIteratorState
{
	Container& container;
	typename Container::iterator iterator;

	TaskContainerIteratorState(Container& container_): container{container_}, iterator(container_.begin())
	{}

	static TaskContainerIterator getIterator()
	{
		return [](void* data) noexcept -> CoreTaskPtr
		{
			auto& state = *reinterpret_cast<TaskContainerIteratorState<Container>*>(data);
			auto& container = state.container;
			auto& iter = state.iterator;

			if (iter == container.end())
			{
				return CoreTaskPtr{};
			}

			CoreTaskPtr coreTask = static_cast<CoreTaskPtr&>(*iter);
			iter++;

			return coreTask;
		};
	}
};

} // namespace cold::async::core
