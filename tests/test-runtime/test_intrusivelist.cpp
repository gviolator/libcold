#pragma once
#include "pch.h"
#include <cold/utils/intrusivelist.h>
#include <cold/utils/strings.h>

using namespace cold;


struct MyValue : IntrusiveListElement<MyValue>
{
	uint64_t value;
	MyValue()
	{
		static uint64_t s_counter = 0;
		value = s_counter++;
	}
	bool operator==(const MyValue& other) const
	{
		return value == other.value;
	}
};

//--iterator actions
//operator++
//operator++(int)
//operator--
//operator--(int)
//const_reference operator*
//reference operator*
//const_pointer operator ->
//pointer operator ->
//operator ==
//operator !=

//IntrusiveList
//--constructors / destructor
//IntrusiveList()
//~IntrusiveList()
//IntrusiveList(const IntrusiveList&)
//IntrusiveList(IntrusiveList&&)
//IntrusiveList& operator = (const IntrusiveList&)
//IntrusiveList& operator = (IntrusiveList&&)

//--iterators
//iterator begin()
//iterator end()
//const_iterator begin()
//const_iterator end()

//--references
//reference back()
//const_reference back()
//reference front()
//const_reference front()

//--markers
//size_t size()
//bool empty()

//--erasers
//void clear()
//void removeElement(T& element)
//iterator erase(iterator& first, iterator last)
//iterator erase(iterator& pos)

//--modifiers
//iterator insert(iterator pos, T& element)
//void push_back(T& element)
//void push_front(T& element)

struct DebugCheckWithExceptionGuard
{
	diagnostics::RuntimeCheck::FailureHandler::UniquePtr prevHandler;



	DebugCheckWithExceptionGuard()
	{
		using namespace cold::diagnostics;

		

		RuntimeCheck::setFailureHandler(RuntimeCheck::FailureHandler::makeUnique([](const SourceInfo&, const wchar_t*, const wchar_t*, const wchar_t*)
		{
			throw std::exception("Failure");
		}), &prevHandler);

	}


	~DebugCheckWithExceptionGuard()
	{
		using namespace cold::diagnostics;

		RuntimeCheck::setFailureHandler(std::move(prevHandler));
	}


	//static void asserFailureWithException(void* , const wchar_t* , const wchar_t* , int , const wchar_t* , const wchar_t* , const wchar_t* message)
	//{
	//	std::string str = StringConv::toUtf8(message);

	//	throw std::exception(str.c_str());
	//}
};



class Test_Common_IntrusiveList : public ::testing::Test
{
	// default assert failure handler should be replaced by exception throwing.
	DebugCheckWithExceptionGuard m_assertionHandlerGuard;

protected:
	template <typename TestedList>
	void TEST_CheckEmptiness(TestedList &originList)
	{
		{//nonConst
			IntrusiveList<MyValue>& list = originList;

			IntrusiveList<MyValue>::iterator listBegin = list.begin();
			IntrusiveList<MyValue>::iterator listEnd = list.end();

			//iterators
			ASSERT_EQ(list.begin(), nullptr);//nonconst
			ASSERT_EQ(list.end(), nullptr);//nonconst

			ASSERT_ANY_THROW(listBegin++);
			ASSERT_ANY_THROW(++listBegin);
			ASSERT_ANY_THROW(listBegin--);
			ASSERT_ANY_THROW(--listBegin);

			ASSERT_ANY_THROW(*listBegin);//nonconst

			ASSERT_ANY_THROW(listBegin.operator->());//nonconst

			ASSERT_TRUE(listBegin == listEnd);
			ASSERT_FALSE(listBegin != listEnd);

			//references
			ASSERT_ANY_THROW(list.front());//nonconst
			ASSERT_ANY_THROW(list.back());//nonconst

										  //markers
			ASSERT_EQ(list.size(), 0);
			ASSERT_TRUE(list.empty());

			//erasers
			ASSERT_NO_THROW(list.clear());
			MyValue notInList{};
			ASSERT_ANY_THROW(list.removeElement(notInList));

			ASSERT_ANY_THROW(list.erase(listBegin, listEnd));

			ASSERT_ANY_THROW(list.erase(listBegin));
			ASSERT_ANY_THROW(list.erase(listEnd));
		}
		{//const
			const IntrusiveList<MyValue>& list = originList;

			IntrusiveList<MyValue>::const_iterator listBegin = list.begin();
			IntrusiveList<MyValue>::const_iterator listEnd = list.end();

			//iterators
			ASSERT_EQ(listBegin, nullptr);//const
			ASSERT_EQ(listEnd, nullptr);//const

			ASSERT_ANY_THROW(*listBegin);//const

			ASSERT_ANY_THROW(listBegin.operator->());//const

			ASSERT_TRUE(listBegin == listEnd);
			ASSERT_FALSE(listBegin != listEnd);

			//references
			ASSERT_ANY_THROW(list.front());//const
			ASSERT_ANY_THROW(list.back());//const

										  //markers
			ASSERT_EQ(list.size(), 0);
			ASSERT_TRUE(list.empty());
		}
		{//mixed
			IntrusiveList<MyValue>& list = originList;
			const IntrusiveList<MyValue>& constList = originList;

			IntrusiveList<MyValue>::iterator begin = list.begin();
			IntrusiveList<MyValue>::iterator end = list.end();

			IntrusiveList<MyValue>::const_iterator constBegin = constList.begin();
			IntrusiveList<MyValue>::const_iterator constEnd = constList.end();

			ASSERT_TRUE(begin == constBegin);
			ASSERT_TRUE(end == constEnd);

			ASSERT_FALSE(begin != constBegin);
			ASSERT_FALSE(end != constEnd);
		}
	}

	template <typename TestedList, typename Val>
	void TEST_CheckSingleElement(TestedList &originList, const Val& val)
	{
		IntrusiveList<MyValue>& list = originList;
		const IntrusiveList<MyValue>& constList = originList;

		IntrusiveList<MyValue>::iterator begin = list.begin();
		IntrusiveList<MyValue>::iterator end = list.end();

		IntrusiveList<MyValue>::const_iterator constBegin = constList.begin();
		IntrusiveList<MyValue>::const_iterator constEnd = constList.end();

		ASSERT_NE(begin, nullptr);
		ASSERT_EQ(end, nullptr);

		//const_reference operator*
		ASSERT_NO_THROW(constBegin.operator->());
		ASSERT_EQ(*constBegin, val);
		ASSERT_ANY_THROW(constEnd.operator*());

		//reference operator*
		ASSERT_NO_THROW(*begin);
		ASSERT_EQ(*begin, val);
		ASSERT_ANY_THROW(*end);

		//const_pointer operator ->
		ASSERT_NO_THROW(constBegin.operator->());
		ASSERT_EQ(constBegin->value, val.value);
		ASSERT_ANY_THROW(constEnd.operator->());

		//pointer operator ->
		ASSERT_NO_THROW(begin.operator->());
		ASSERT_EQ(begin->value, val.value);
		ASSERT_ANY_THROW(end.operator->());

		//operator ==
		ASSERT_FALSE(begin == end);
		ASSERT_FALSE(constBegin == constEnd);

		ASSERT_TRUE(begin == constBegin);
		ASSERT_TRUE(end == constEnd);

		//operator !=
		ASSERT_TRUE(begin != end);
		ASSERT_TRUE(constBegin != constEnd);

		ASSERT_FALSE(begin != constBegin);
		ASSERT_FALSE(end != constEnd);

		//operator++(int)
		{
			IntrusiveList<MyValue>::iterator beginPP = list.begin();
			IntrusiveList<MyValue>::iterator endPP = list.end();

			ASSERT_NO_THROW(beginPP++);
			ASSERT_ANY_THROW(endPP++);

			ASSERT_TRUE(beginPP == end);
		}
		{
			IntrusiveList<MyValue>::iterator beginPP = list.begin();
			IntrusiveList<MyValue>::iterator iter = beginPP++;
			ASSERT_FALSE(iter == end);

			ASSERT_TRUE(beginPP == end);
		}

		//operator++
		{
			IntrusiveList<MyValue>::iterator PPbegin = list.begin();
			IntrusiveList<MyValue>::iterator PPend = list.end();

			ASSERT_NO_THROW(++PPbegin);

			ASSERT_TRUE(PPbegin == end);

			ASSERT_ANY_THROW(++PPend);
		}
		{
			IntrusiveList<MyValue>::iterator PPbegin = list.begin();
			IntrusiveList<MyValue>::iterator iter = ++PPbegin;
			ASSERT_TRUE(iter == end);

			ASSERT_TRUE(PPbegin == end);
		}

		//operator--(int)
		{
			IntrusiveList<MyValue>::iterator beginMM = list.begin();
			IntrusiveList<MyValue>::iterator endMM = list.end();

			ASSERT_ANY_THROW(endMM--);

			ASSERT_FALSE(beginMM == endMM);

			ASSERT_ANY_THROW(beginMM--);
		}
		//operator--
		{
			IntrusiveList<MyValue>::iterator MMbegin = list.begin();
			IntrusiveList<MyValue>::iterator MMend = list.end();

			ASSERT_ANY_THROW(--MMend);

			ASSERT_FALSE(MMbegin == MMend);

			ASSERT_ANY_THROW(--MMbegin);
		}

		//references
		ASSERT_NO_THROW(list.front());
		ASSERT_NO_THROW(list.back());
		ASSERT_NO_THROW(constList.front());
		ASSERT_NO_THROW(constList.back());

		ASSERT_EQ(list.front(), val);
		ASSERT_EQ(list.back(), val);
		ASSERT_EQ(constList.front(), val);
		ASSERT_EQ(constList.back(), val);

		//markers
		ASSERT_EQ(list.size(), 1);
		ASSERT_FALSE(list.empty());
		ASSERT_EQ(constList.size(), 1);
		ASSERT_FALSE(constList.empty());
	}

	template <typename TestedList, typename SecElementIter,typename FirstVal, typename SecVal>
	void TEST_CheckIterDecrement(TestedList& originList, SecElementIter secElementIter,const FirstVal& frontVal, const SecVal& secVal)
	{
		ASSERT_EQ(originList.size(), 2);

		IntrusiveList<MyValue>::iterator iter = secElementIter;

		ASSERT_TRUE(*secElementIter == secVal);
		ASSERT_TRUE(secElementIter->value == secVal.value);

		{//operator--
			IntrusiveList<MyValue>::iterator MMiter = iter;
			IntrusiveList<MyValue>::iterator MMfirstElementIter = --MMiter;

			ASSERT_TRUE(*MMfirstElementIter == frontVal);
			ASSERT_TRUE(MMfirstElementIter->value == frontVal.value);

			ASSERT_TRUE(*MMfirstElementIter == frontVal);
			ASSERT_TRUE(MMfirstElementIter->value == frontVal.value);

			ASSERT_TRUE(MMfirstElementIter == MMiter);
			ASSERT_FALSE(MMfirstElementIter != MMiter);
		}
		{//operator--(int)
			IntrusiveList<MyValue>::iterator firstElementIterMM = iter;
			IntrusiveList<MyValue>::iterator iterMM = firstElementIterMM--;

			ASSERT_TRUE(*firstElementIterMM == frontVal);
			ASSERT_TRUE(firstElementIterMM->value == frontVal.value);

			ASSERT_TRUE(*iterMM == secVal);
			ASSERT_TRUE(iterMM->value == secVal.value);

			ASSERT_TRUE(firstElementIterMM != iterMM);
			ASSERT_FALSE(firstElementIterMM == iterMM);
		}
	}

	template <typename TestedList, typename LastElementIter, typename IterVal>
	void TEST_CheckLastIter(TestedList& originlist, LastElementIter lastElementIter, const IterVal& val)
	{
		ASSERT_TRUE(originlist.size() > 1);

		IntrusiveList<MyValue>& list = originlist;
		const IntrusiveList<MyValue>& constList = originlist;

		{
			IntrusiveList<MyValue>::iterator iter = lastElementIter;
			IntrusiveList<MyValue>::const_iterator constIter = lastElementIter;

			ASSERT_TRUE(list.back() == *iter);
			ASSERT_TRUE(list.back() == *constIter);

			ASSERT_TRUE(constList.back() == *iter);
			ASSERT_TRUE(constList.back() == *constIter);

			ASSERT_TRUE(list.back() == val);
			ASSERT_TRUE(constList.back() == val);

			ASSERT_TRUE(iter->value == val.value);
		}

		{
			IntrusiveList<MyValue>::iterator iter = lastElementIter;
			IntrusiveList<MyValue>::iterator PPiter = ++iter;
			ASSERT_TRUE(PPiter == iter);
			ASSERT_TRUE(PPiter == list.end());
			ASSERT_TRUE(iter == list.end());
		}
		{
			IntrusiveList<MyValue>::iterator iter = lastElementIter;
			IntrusiveList<MyValue>::iterator iterPP = iter++;
			ASSERT_TRUE(iterPP != iter);
			ASSERT_TRUE(iterPP != list.end());
			ASSERT_TRUE(iterPP == lastElementIter);
			ASSERT_TRUE(iter == list.end());
		}
	}

	template <typename TestedList, typename FirstElementIter, typename IterVal>
	void TEST_CheckFirstIter(TestedList& originlist, FirstElementIter firstElementIter, const IterVal& val)
	{
		ASSERT_TRUE(originlist.size() > 1);

		IntrusiveList<MyValue>& list = originlist;
		const IntrusiveList<MyValue>& constList = originlist;

		ASSERT_TRUE(list.begin() == firstElementIter);
		ASSERT_TRUE(constList.begin() == firstElementIter);

		IntrusiveList<MyValue>::iterator iter = firstElementIter;
		IntrusiveList<MyValue>::const_iterator constIter = firstElementIter;

		ASSERT_TRUE(*iter == val);
		ASSERT_TRUE(*constIter == val);

		ASSERT_TRUE(iter->value == val.value);
		ASSERT_TRUE(constIter->value == val.value);

		ASSERT_ANY_THROW(iter--);
		ASSERT_TRUE(iter == firstElementIter);
		ASSERT_TRUE(iter == constIter);

		ASSERT_ANY_THROW(--iter);
		ASSERT_TRUE(iter == firstElementIter);
		ASSERT_TRUE(iter == constIter);

		IntrusiveList<MyValue>::iterator iterPP = iter;
		IntrusiveList<MyValue>::iterator PPiter = iter;

		IntrusiveList<MyValue>::iterator firstIter = iterPP++;
		ASSERT_TRUE(firstIter != iterPP);
		ASSERT_TRUE(firstIter == iter);

		IntrusiveList<MyValue>::iterator nextIter = ++PPiter;
		ASSERT_TRUE(nextIter == iterPP);
		ASSERT_TRUE(nextIter != iter);

		ASSERT_TRUE(iterPP != iter);
		ASSERT_TRUE(PPiter != iter);

		ASSERT_TRUE(PPiter == iterPP);
	}

	template <typename Iter, typename IterVal>
	void TEST_IterIsPresent(Iter iter, const IterVal& val)
	{
		ASSERT_NO_THROW(iter.operator*());
		ASSERT_NO_THROW(iter.operator->());
		ASSERT_TRUE(*iter == val);
		ASSERT_TRUE(iter->value == val.value);
	}
};




/// <summary>
///
/// </summary>
/// <param name=""></param>
/// <param name=""></param>
/// <returns></returns>
TEST_F(Test_Common_IntrusiveList, CheckEmptiness)
{
	IntrusiveList<MyValue> originList;

	ASSERT_NO_FATAL_FAILURE(TEST_CheckEmptiness(originList));
}

/// <summary>
///
/// </summary>
/// <param name=""></param>
/// <param name=""></param>
/// <returns></returns>
TEST_F(Test_Common_IntrusiveList, CheckAddingRemovingElement)
{
	{
		IntrusiveList<MyValue> originList;

		{//check push_back
			MyValue val;

			ASSERT_NO_THROW(originList.push_back(val));

			//check single element
			ASSERT_NO_FATAL_FAILURE(TEST_CheckSingleElement(originList, val));
		}
		//the local object (val) was destroyed, and automatically was removed from the originList
		//check emptiness
		ASSERT_NO_FATAL_FAILURE(TEST_CheckEmptiness(originList));
	}
	{
		IntrusiveList<MyValue> originList;

		{//check push_front
			MyValue val;

			ASSERT_NO_THROW(originList.push_front(val));

			//check single element
			ASSERT_NO_FATAL_FAILURE(TEST_CheckSingleElement(originList, val));
		}
		//the local object (val) was destroyed, and automatically was removed from the originList
		//check emptiness
		ASSERT_NO_FATAL_FAILURE(TEST_CheckEmptiness(originList));
	}

	{
		IntrusiveList<MyValue> originList;

		{//check insertion
			MyValue val;

			ASSERT_NO_THROW(originList.insert(originList.begin(), val));

			//check single element
			ASSERT_NO_FATAL_FAILURE(TEST_CheckSingleElement(originList, val));
		}
		//the local object (val) was destroyed, and automatically was removed from the originList
		//check emptiness
		ASSERT_NO_FATAL_FAILURE(TEST_CheckEmptiness(originList));
	}
}

/// <summary>
///
/// </summary>
/// <param name=""></param>
/// <param name=""></param>
/// <returns></returns>
TEST_F(Test_Common_IntrusiveList, CheckErasers)
{
	IntrusiveList<MyValue> originList;

	{//check removeElement
		MyValue val;

		ASSERT_NO_THROW(originList.insert(originList.begin(), val));

		ASSERT_NO_THROW(originList.removeElement(val));

		ASSERT_NO_FATAL_FAILURE(TEST_CheckEmptiness(originList));

	}

	{
		MyValue val;

		ASSERT_NO_THROW(originList.insert(originList.begin(), val));

		ASSERT_NO_THROW(originList.clear());

		ASSERT_NO_FATAL_FAILURE(TEST_CheckEmptiness(originList));

	}

	{
		MyValue val;

		ASSERT_NO_THROW(originList.insert(originList.begin(), val));

		ASSERT_NO_THROW(originList.erase(originList.begin()));

		ASSERT_NO_FATAL_FAILURE(TEST_CheckEmptiness(originList));

	}
	{
		MyValue val;

		ASSERT_NO_THROW(originList.insert(originList.begin(), val));

		ASSERT_NO_THROW(originList.erase(originList.begin(), originList.end()));

		std::list<int> lst;
		lst.push_back(1);
		lst.erase(lst.begin(), lst.end());

		ASSERT_NO_FATAL_FAILURE(TEST_CheckEmptiness(originList));
	}
}

/// <summary>
///
/// </summary>
/// <param name=""></param>
/// <param name=""></param>
/// <returns></returns>
TEST_F(Test_Common_IntrusiveList, CheckDoubleAdding)
{
	IntrusiveList<MyValue> originList;

	{
		MyValue val;

		IntrusiveList<MyValue>::iterator firstInsertRetIter;
		ASSERT_NO_THROW(firstInsertRetIter = originList.insert(originList.end(), val));
		ASSERT_NE(firstInsertRetIter, nullptr);
		ASSERT_TRUE(firstInsertRetIter == originList.begin());

		{//insert in to front
			MyValue& lastVal = val;
			MyValue frontVal;

			IntrusiveList<MyValue>::iterator insertRetIter;

			ASSERT_EQ(originList.size(), 1);
			ASSERT_NO_THROW(insertRetIter = originList.insert(originList.begin(), frontVal));
			ASSERT_NE(insertRetIter, nullptr);
			ASSERT_NO_THROW(insertRetIter.operator*());
			ASSERT_NO_THROW(insertRetIter.operator->());
			ASSERT_TRUE(*insertRetIter == frontVal);
			ASSERT_TRUE(insertRetIter->value == frontVal.value);

			ASSERT_EQ(originList.size(), 2);
			ASSERT_TRUE(insertRetIter == originList.begin());

			{
				IntrusiveList<MyValue>::iterator iter = insertRetIter;

				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, iter, frontVal));

				//operator++
				IntrusiveList<MyValue>::iterator PPsecondElementIter = ++iter;
				ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(originList, PPsecondElementIter, lastVal));
				ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(originList, iter, lastVal));

				//operator--
				IntrusiveList<MyValue>::iterator MMfirstElementIter = --iter;
				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, MMfirstElementIter, frontVal));
				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, iter, frontVal));

				//operator++(int)
				IntrusiveList<MyValue>::iterator iterPP = iter++;
				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, iterPP, frontVal));
				ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(originList, iter, lastVal));

				//operator--(int)
				IntrusiveList<MyValue>::iterator MMiter = iter--;
				ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(originList, MMiter, lastVal));
				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, iter, frontVal));
			}
			{//
				MyValue& sameValue = lastVal;

				ASSERT_ANY_THROW(originList.insert(originList.end(), sameValue));
				ASSERT_ANY_THROW(originList.insert(originList.begin(), sameValue));
				ASSERT_ANY_THROW(originList.push_front(sameValue));
				ASSERT_ANY_THROW(originList.push_back(sameValue));
			}
			{//
				MyValue& sameValue = frontVal;

				ASSERT_ANY_THROW(originList.insert(originList.end(), sameValue));
				ASSERT_ANY_THROW(originList.insert(originList.begin(), sameValue));
				ASSERT_ANY_THROW(originList.push_front(sameValue));
				ASSERT_ANY_THROW(originList.push_back(sameValue));
			}
		}
		ASSERT_NO_FATAL_FAILURE(TEST_CheckSingleElement(originList, val));

		{// insert in to end
			MyValue& frontVal = val;
			MyValue lastVal;

			IntrusiveList<MyValue>::iterator insertRetIter;

			ASSERT_EQ(originList.size(), 1);

			ASSERT_NO_THROW(insertRetIter = originList.insert(originList.end(), lastVal));
			ASSERT_NE(insertRetIter, nullptr);

			ASSERT_NO_THROW(insertRetIter.operator*());
			ASSERT_NO_THROW(insertRetIter.operator->());
			ASSERT_TRUE(*insertRetIter == lastVal);
			ASSERT_TRUE(insertRetIter->value == lastVal.value);

			ASSERT_EQ(originList.size(), 2);

			{
				IntrusiveList<MyValue>::iterator iter = insertRetIter;

				ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(originList, iter, lastVal));

				//operator++
				IntrusiveList<MyValue>::iterator PPsecondElementIter = iter;
				ASSERT_TRUE(++PPsecondElementIter == originList.end());
				ASSERT_TRUE(PPsecondElementIter == originList.end());

				//operator--
				IntrusiveList<MyValue>::iterator MMfirstElementIter = --iter;
				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, MMfirstElementIter, frontVal));
				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, iter, frontVal));

				//operator++(int)
				IntrusiveList<MyValue>::iterator iterPP = iter++;
				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, iterPP, frontVal));
				ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(originList, iter, lastVal));

				//operator--(int)
				IntrusiveList<MyValue>::iterator MMiter = iter--;
				ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(originList, MMiter, lastVal));
				ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(originList, iter, frontVal));
			}
			{//try to insert lastVal again
				MyValue& sameValue = lastVal;

				ASSERT_ANY_THROW(originList.insert(originList.end(), sameValue));
				ASSERT_ANY_THROW(originList.insert(originList.begin(), sameValue));
				ASSERT_ANY_THROW(originList.push_front(sameValue));
				ASSERT_ANY_THROW(originList.push_back(sameValue));
			}
			{//try to insert lastVal again
				MyValue& sameValue = frontVal;

				ASSERT_ANY_THROW(originList.insert(originList.end(), sameValue));
				ASSERT_ANY_THROW(originList.insert(originList.begin(), sameValue));
				ASSERT_ANY_THROW(originList.push_front(sameValue));
				ASSERT_ANY_THROW(originList.push_back(sameValue));
			}
		}
		ASSERT_NO_FATAL_FAILURE(TEST_CheckSingleElement(originList, val));

		{//push to front
			MyValue& lastVal = val;
			MyValue frontVal;

			ASSERT_TRUE(originList.front() == val);
			ASSERT_EQ(originList.size(), 1);
			ASSERT_NO_THROW(originList.push_front(frontVal));
			ASSERT_EQ(originList.size(), 2);
			ASSERT_TRUE(originList.front() == frontVal);
		}
		ASSERT_NO_FATAL_FAILURE(TEST_CheckSingleElement(originList, val));

		{//push to front
			MyValue lastVal;
			MyValue& frontVal = val;

			ASSERT_TRUE(originList.back() == val);
			ASSERT_EQ(originList.size(), 1);
			ASSERT_NO_THROW(originList.push_back(lastVal));
			ASSERT_EQ(originList.size(), 2);
			ASSERT_TRUE(originList.back() == lastVal);
		}
		ASSERT_NO_FATAL_FAILURE(TEST_CheckSingleElement(originList, val));

		{//try to insert single again
			MyValue& sameValue = val;

			ASSERT_ANY_THROW(originList.insert(originList.end(), sameValue));
			ASSERT_ANY_THROW(originList.insert(originList.begin(), sameValue));
			ASSERT_ANY_THROW(originList.push_back(sameValue));
			ASSERT_ANY_THROW(originList.push_front(sameValue));
		}
	}
}

/// <summary>
///
/// </summary>
/// <param name=""></param>
/// <param name=""></param>
/// <returns></returns>
TEST_F(Test_Common_IntrusiveList, CheckMiddleRemoving)
{
	MyValue val1;
	MyValue val3;

	IntrusiveList<MyValue> list;

	IntrusiveList<MyValue>::iterator iter1;
	IntrusiveList<MyValue>::iterator iter2;
	IntrusiveList<MyValue>::iterator iter3;
	{
		MyValue val2;

		ASSERT_NO_THROW(iter1 = list.insert(list.begin(), val1));
		ASSERT_NO_THROW(iter2 = list.insert(list.end(), val2));
		ASSERT_NO_THROW(iter3 = list.insert(list.end(), val3));

		ASSERT_EQ(list.size(), 3);

		ASSERT_NO_FATAL_FAILURE(TEST_IterIsPresent(iter1, val1));
		ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(list, iter1, val1));

		ASSERT_NO_FATAL_FAILURE(TEST_IterIsPresent(iter2, val2));

		ASSERT_NO_FATAL_FAILURE(TEST_IterIsPresent(iter3, val3));
		ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(list, iter3, val3));

		{
			IntrusiveList<MyValue>::iterator iter = list.begin();
			ASSERT_TRUE(*iter == val1);
			ASSERT_TRUE(*++iter == val2);
			ASSERT_TRUE(*++iter == val3);
		}
		//here val2 was destroyed
	}
	ASSERT_NO_FATAL_FAILURE(TEST_IterIsPresent(iter1, val1));
	ASSERT_NO_FATAL_FAILURE(TEST_CheckFirstIter(list, iter1, val1));

	//todo: what way should the broken iterator behave?
	//it is a vulnerability
//	ASSERT_ANY_THROW(iter2.operator*());
//	ASSERT_ANY_THROW(iter2.operator->());

	ASSERT_NO_FATAL_FAILURE(TEST_IterIsPresent(iter3, val3));
	ASSERT_NO_FATAL_FAILURE(TEST_CheckLastIter(list, iter3, val3));

	{
		IntrusiveList<MyValue>::iterator iter = list.begin();
		ASSERT_TRUE(*iter == val1);
		ASSERT_TRUE(*++iter == val3);
	}


	IntrusiveList<MyValue>::iterator insertRetIter;
}


/// <summary>
///
/// </summary>
TEST_F(Test_Common_IntrusiveList, Move)
{
	MyValue val1;
	MyValue val2;

	IntrusiveList<MyValue> list;

	list.push_back(val1);
	list.push_back(val2);

	IntrusiveList<MyValue> list2 = std::move(list);

	ASSERT_TRUE(list.empty());
	ASSERT_EQ(list2.size(), 2);

	auto iter = list2.begin();
	ASSERT_EQ(&(*iter), &val1);
	ASSERT_EQ(&(*++iter), &val2);
}


