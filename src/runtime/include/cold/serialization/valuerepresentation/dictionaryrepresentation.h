#pragma once
#include "representationbase.h"
#include <cold/meta/concepts.h>

namespace cold::meta {

template<typename T>
struct DictionaryValueOperations;

template<typename T>
concept DictionaryRepresentable =
	requires(const T& dict)
	{
		typename DictionaryValueOperations<T>::Key;
		typename DictionaryValueOperations<T>::Value;

		{DictionaryValueOperations<T>::size(dict)} -> concepts::AssignableTo<size_t&>;
		{DictionaryValueOperations<T>::key(dict, size_t{0})} -> concepts::AssignableTo<typename DictionaryValueOperations<T>::Key&>;
		{DictionaryValueOperations<T>::find(dict, std::declval<typename DictionaryValueOperations<T>::Key>(), (std::add_const_t<typename DictionaryValueOperations<T>::Value*>*)nullptr)} -> concepts::Same<bool>;
	} &&
	requires(T& dict)
	{
		DictionaryValueOperations<T>::clear(dict);
		{DictionaryValueOperations<T>::emplace(dict, std::declval<typename DictionaryValueOperations<T>::Key>())} -> concepts::AssignableTo<typename DictionaryValueOperations<T>::Value&>;
		{DictionaryValueOperations<T>::find(dict, std::declval<typename DictionaryValueOperations<T>::Key>(), (typename DictionaryValueOperations<T>::Value**)nullptr)} -> concepts::Same<bool>;
	};


namespace meta_internal {




}


}

