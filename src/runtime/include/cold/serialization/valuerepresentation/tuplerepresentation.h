#pragma once
#include "representationbase.h"
#include <cold/meta/concepts.h>

namespace cold::meta {

template<typename>
struct TupleValueOperations;

template<typename T>
concept TupleRepresentable =
	requires(const T& tup)
	{
		{TupleValueOperations<T>::TupleSize} -> concepts::AssignableTo<size_t&>;
	};

/**
*/
template<TupleRepresentable T>
class TupleRepresentation final : public RepresentationBase<T, T>
{
public:
	static constexpr size_t TupleSize = TupleValueOperations<T>::TupleSize;

	TupleRepresentation(T& tup): m_tuple(tup)
	{}

	template<size_t Idx>
	requires (Idx < TupleValueOperations<T>::TupleSize)
	auto element() const
	{
		decltype(auto) el = TupleValueOperations<T>::template element<Idx>(m_tuple);
		static_assert(std::is_reference_v<decltype(el)>);
		return represent(el);
	}

private:

	T& m_tuple;
};

/**
*/
template<TupleRepresentable T>
class ConstTupleRepresentation final : public RepresentationBase<T, T>
{
public:
	static constexpr size_t TupleSize = TupleValueOperations<T>::TupleSize;

	ConstTupleRepresentation(const T& tup): m_tuple(tup)
	{}

	template<size_t Idx>
	requires (Idx < TupleValueOperations<T>::TupleSize)
	auto element() const
	{
		decltype(auto) el = TupleValueOperations<T>::template element<Idx>(m_tuple);
		static_assert(std::is_reference_v<decltype(el)>);
		return represent(el);
	}

private:
	const T& m_tuple;
};

//-----------------------------------------------------------------------------

template<TupleRepresentable T>
TupleRepresentation<T> represent(T& tup)
{
	return tup;
}

template<TupleRepresentable T>
ConstTupleRepresentation<T> represent(const T& tup)
{
	return tup;
}

} // namespace cold::meta
