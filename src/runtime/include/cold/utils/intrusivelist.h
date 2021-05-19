#pragma once
#include <cold/diagnostics/runtimecheck.h>
#include <iterator>


namespace cold {


template<typename T/*, typename Mutex = threading::CriticalSection*/>
class IntrusiveList;

template<typename T>
class IntrusiveListElement;


/// <summary>
/// Represents intrusive list: where each element stored outside of the list, but is an IntrusiveListElement.
/// </summary>
template<typename T/*, typename Mutex*/>
class IntrusiveList
{
public:

	using Node = IntrusiveListElement<T>;
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;


	struct iterator
	{
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = T&;


		Node* node;

		iterator(Node* node_ = nullptr)
			: node(node_)
		{}

		iterator& operator++()
		{
			DEBUG_CHECK(node != nullptr, "can not be increment iterator");

			node = node->m_next;

			return *this;
		}

		iterator operator++(int)
		{
			DEBUG_CHECK(node != nullptr, "can not be increment iterator");

			iterator retIter(node);

			node = node->m_next;

			return retIter;
		}

		iterator& operator--()
		{
			DEBUG_CHECK(node != nullptr, "can not be decrement iterator");

			DEBUG_CHECK(node->m_prev != nullptr, "can not be decrement iterator");

			node = node->m_prev;

			return *this;
		}

		iterator operator--(int)
		{
			DEBUG_CHECK(node != nullptr, "can not be decrement iterator");

			DEBUG_CHECK(node->m_prev != nullptr, "can not be decrement iterator");

			iterator retIter(node);

			node = node->m_prev;

			return retIter;
		}

		const_reference operator*() const
		{
			DEBUG_CHECK(node != nullptr, "intrusive list iterator can not be dereferenced");

			return *static_cast<T*>(node);
		}

		reference operator*()
		{
			DEBUG_CHECK(node != nullptr, "intrusive list iterator can not be dereferenced");

			return *static_cast<T*>(node);
		}

		pointer operator -> ()
		{
			DEBUG_CHECK(node != nullptr, "intrusive list iterator can not be dereferenced");

			return static_cast<T*>(node);
		}

		const_pointer operator -> () const
		{
			DEBUG_CHECK(node != nullptr, "intrusive list iterator can not be dereferenced");

			return static_cast<T*>(node);
		}

		friend bool operator == (const iterator& iter1, const iterator& iter2)
		{
			return iter1.node == iter2.node;
		}

		friend bool operator != (const iterator& iter1, const iterator& iter2)
		{
			return iter1.node != iter2.node;
		}
	};


	using const_iterator = iterator;


	IntrusiveList() noexcept;

	~IntrusiveList() noexcept;

	IntrusiveList(const IntrusiveList&) = delete;

	IntrusiveList(IntrusiveList&&);

	IntrusiveList& operator = (const IntrusiveList&) = delete;

	IntrusiveList& operator = (IntrusiveList&&);


	/// <summary>
	/// Clear intrusive list.
	/// </summary>
	void clear() noexcept;


	/// <summary>
	/// Compute size of the linked list.
	/// </summary>
	/// <returns>How many elements contains within list.</returns>
	size_t size() const noexcept;

	/// <summary>
	/// DEBUG_CHECK that list is empty.
	/// </summary>
	/// <returns>true if list contains no element, false otherwise</returns>
	bool empty() const noexcept;

	/// <summary>
	///
	/// </summary>
	/// <returns>Reference to the last list element</returns>
	reference back();

	/// <summary>
	///
	/// </summary>
	/// <returns>Constant reference to the last list element</returns>
	const_reference back() const;

	/// <summary>
	///
	/// </summary>
	/// <returns>Constant reference to the first list element</returns>
	reference front();


	/// <summary>
	///
	/// </summary>
	/// <returns>Constant reference to the first list element</returns>
	const_reference front() const;


	/// <summary>
	/// Place element at the end of the list.
	/// </summary>
	/// <param name="element">Inserted element</param>
	void push_back(T& element);

	/// <summary>
	/// Place element at the front of the list.
	/// </summary>
	/// <param name="element">Inserted element</param>
	void push_front(T& element);


	/// <summary>
	/// Insert value before pos.
	/// </summary>
	/// <param name="pos">Where to insert</param>
	/// <param name="node">Inserted element</param>
	/// <returns>Iterator pointing to the inserted value.</returns>
	iterator insert(iterator pos, T& element);


	/// <summary>
	///
	/// </summary>
	/// <param name="first"></param>
	/// <param name="last"></param>
	/// <returns></returns>
	iterator erase(iterator first, iterator last);


	/// <summary>
	/// Removes specified element (referenced by iterator) from the list.
	/// </summary>
	/// <param name="pos">Removed element (iterator).</param>
	/// <returns>Iterator following the last removed element. If the iterator pos refers to the last element, the end() iterator is returned. </returns>
	iterator erase(iterator pos);


	/// <summary>
	///
	/// </summary>
	/// <param name="element"></param>
	void removeElement(T& element);


	iterator begin();

	iterator end();

	const_iterator begin() const;

	const_iterator end() const;

private:

	static Node& asFreeNode(T& element);

	void moveFrom(IntrusiveList&& list);


	Node* m_head = nullptr;
	Node* m_tail = nullptr;
};


/// <summary>
/// In order for an instance of a class to be placed in an intrusive list, it must inherit from IntrusiveListElement.
/// </summary>
template<typename T>
class IntrusiveListElement
{
protected:

	~IntrusiveListElement()
	{
		static_assert(std::is_base_of_v<IntrusiveListElement<T>, T>, "Type is not IntrusiveListElement");

		if (m_list)
		{
			m_list->removeElement(static_cast<T&>(*this));
		}
	}

private:

	void clearListElementState()
	{
		m_list = nullptr;
		m_prev = nullptr;
		m_next = nullptr;
	}


	IntrusiveList<T>* m_list = nullptr;
	IntrusiveListElement* m_prev = nullptr;
	IntrusiveListElement* m_next = nullptr;

	template<typename/*, typename*/>
	friend class IntrusiveList;
};


//-----------------------------------------------------------------------------
template<typename T>
IntrusiveList<T>::IntrusiveList() noexcept
{
	static_assert(std::is_base_of_v<IntrusiveListElement<T>, T>, "Type is not IntrusiveListElement");
}

template<typename T>
void IntrusiveList<T>::moveFrom(IntrusiveList&& list)
{
	if (&list == this)
	{
		return ;
	}

	clear();

	DEBUG_CHECK(m_head == nullptr && m_tail == nullptr);

	std::swap(m_head, list.m_head);
	std::swap(m_tail, list.m_tail);

	auto node = m_head;

	while (node)
	{
		node->m_list = this;
		node = node->m_next;
	}
}


template<typename T>
IntrusiveList<T>::IntrusiveList(IntrusiveList&& list)
{
	moveFrom(std::move(list));
}


template<typename T>
IntrusiveList<T>& IntrusiveList<T>::operator = (IntrusiveList&& list)
{
	moveFrom(std::move(list));

	return *this;
}


template<typename T>
IntrusiveList<T>::~IntrusiveList() noexcept
{
	clear();
}


template<typename T>
void IntrusiveList<T>::clear() noexcept
{
	auto node = m_tail;

	while (node != nullptr)
	{
		auto prev = node->m_prev;

		node->clearListElementState();

		node = prev;
	}

	m_head = nullptr;

	m_tail = m_head;
}


template<typename T>
size_t IntrusiveList<T>::size() const noexcept
{
	size_t counter = 0;

	auto node = m_head;

	while (node != nullptr)
	{
		++counter;

		node = node->m_next;
	}

	return counter;
}


template<typename T>
bool IntrusiveList<T>::empty() const noexcept
{
	return m_head == nullptr;
}


template<typename T>
typename IntrusiveList<T>::reference IntrusiveList<T>::back()
{
	DEBUG_CHECK(m_tail != nullptr, "list is empty (or invalid state)");

	return *static_cast<T*>(m_tail);
}


template<typename T>
typename IntrusiveList<T>::const_reference IntrusiveList<T>::back() const
{
	DEBUG_CHECK(m_tail != nullptr, "list is empty (or invalid state)");

	return *static_cast<const T*>(m_tail);
}


template<typename T>
typename IntrusiveList<T>::reference IntrusiveList<T>::front()
{
	DEBUG_CHECK(m_head != nullptr, "list is empty (or invalid state)");

	return *static_cast<T*>(m_head);
}


template<typename T>
typename IntrusiveList<T>::const_reference IntrusiveList<T>::front() const
{
	DEBUG_CHECK(m_head != nullptr, "list is empty (or invalid state)");

	return *static_cast<const T*>(m_head);
}


template<typename T>
void IntrusiveList<T>::push_back(T& element)
{
	Node& node = asFreeNode(element);

	node.m_list = this;

	if (m_head == nullptr)
	{
		DEBUG_CHECK(m_tail == nullptr);

		m_head = &node;
		m_tail = m_head;
	}
	else
	{
		DEBUG_CHECK(m_tail != nullptr);

		m_tail->m_next = &node;

		node.m_prev = m_tail;
		m_tail = &node;
	}
}


template<typename T>
void IntrusiveList<T>::push_front(T& element)
{
	[[maybe_unused]] auto _ = insert(this->begin(), element);
}


template<typename T>
typename IntrusiveList<T>::iterator IntrusiveList<T>::insert(iterator pos, T& element)
{
	DEBUG_CHECK (pos.node == nullptr || pos.node->m_list == this, "Invalid iterator");

	if (pos.node == nullptr)
	{//end
		push_back(element);

		return iterator{m_tail};
	}

	Node& node = asFreeNode(element);

	node.m_list = this;

	node.m_next = pos.node;

	node.m_prev = pos.node->m_prev;

	if (node.m_prev != nullptr)
	{
		node.m_prev->m_next = &node;
	}

	pos.node->m_prev = &node;

	if (pos.node == m_head)
	{
		m_head = &node;
	}

	return iterator{&node};
}


template<typename T>
typename IntrusiveList<T>::iterator IntrusiveList<T>::erase(iterator first, iterator last)
{
	DEBUG_CHECK(first.node != nullptr, "Iterator can not be dereferenced");

	DEBUG_CHECK(first.node->m_list == this, "Invalid list reference. Possible element pointed by iterator already removed from list.");

	if (last == end())
	{
		//remove all form first to end
		iterator nextAfterRemoved = first;

		do
		{
			nextAfterRemoved = erase(nextAfterRemoved);
		} while (nextAfterRemoved != end());

		return end();
	}
	else
	{
		//DEBUG_CHECK than the last is belong to this list
		DEBUG_CHECK(last.node != nullptr, "Iterator can not be dereferenced");

		DEBUG_CHECK(last.node->m_list == this, "Invalid list reference. Possible element pointed by iterator already removed from list.");

		//DEBUG_CHECK that the last stays after the first
		bool lastStaysAfterFirst = false;

		iterator firstPP = first;
		while (++firstPP != end())
		{
			if (firstPP == last)
			{
				lastStaysAfterFirst = true;
				break;
			}
		}
		DEBUG_CHECK(lastStaysAfterFirst, "The last iterator doesn't stay before the first one");

		//remove all elements from the first to the last (the last must stay in the list)
		iterator nextAfterRemoved = first;

		do
		{
			nextAfterRemoved = erase(nextAfterRemoved);
		} while (nextAfterRemoved != last);

		return last;
	}
}


template<typename T>
typename IntrusiveList<T>::iterator IntrusiveList<T>::erase(iterator pos)
{
	DEBUG_CHECK(pos.node != nullptr, "Iterator can not be dereferenced");
	DEBUG_CHECK(pos.node->m_list == this, "Invalid list reference. Possible element pointed by iterator already removed from list.");

	auto prev = pos.node->m_prev;
	auto next = pos.node->m_next;

	if (prev != nullptr)
	{
		DEBUG_CHECK(prev->m_next == pos.node);

		prev->m_next = next;
	}
	else
	{// head is erased
		m_head = next;
	}

	if (next != nullptr)
	{
		DEBUG_CHECK(next->m_prev == pos.node);

		next->m_prev = prev;
	}
	else
	{// tail is erased
		m_tail = prev;
	}

	pos.node->clearListElementState();

	pos.node = nullptr;

	return iterator{next};
}


template<typename T>
void IntrusiveList<T>::removeElement(T& element)
{
	auto& node = static_cast<Node&>(element);

	[[maybe_unused]] auto _ = erase(iterator{&node});
}


template<typename T>
typename IntrusiveList<T>::iterator IntrusiveList<T>::begin()
{
	return iterator{m_head};
}


template<typename T>
typename IntrusiveList<T>::iterator IntrusiveList<T>::end()
{
	return iterator {};
}


template<typename T>
typename IntrusiveList<T>::const_iterator IntrusiveList<T>::begin() const
{
	return const_iterator{m_head};
}


template<typename T>
typename IntrusiveList<T>::const_iterator IntrusiveList<T>::end() const
{
	return const_iterator {};
}


template<typename T>
typename IntrusiveList<T>::Node& IntrusiveList<T>::asFreeNode(T& element)
{
	auto& node = static_cast<Node&>(element);

	DEBUG_CHECK(node.m_next == nullptr && node.m_prev == nullptr && node.m_list == nullptr, "Node already within list");

	return node;
}

}


namespace std {


template<typename T>
decltype(auto) begin(cold::IntrusiveList<T>& container)
{
	return container.begin();
}


template<typename T>
decltype(auto) begin(const cold::IntrusiveList<T>& container)
{
	return container.begin();
}


template<typename T>
decltype(auto) end(cold::IntrusiveList<T>& container)
{
	return container.end();
}


template<typename T>
decltype(auto) end(const cold::IntrusiveList<T>& container)
{
	return container.end();
}

}
