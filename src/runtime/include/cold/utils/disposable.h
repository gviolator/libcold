#pragma once
#include <cold/com/interface.h>
#include <cold/utils/scopeguard.h>

#include <type_traits>

namespace cold {


struct ABSTRACT_TYPE Disposable
{
	virtual void dispose() = 0;
};


template<typename T>
inline void dispose(T& disposable) requires(std::is_assignable_v<Disposable&, T&>)
{
	static_cast<Disposable&>(disposable).dispose();
}



template<typename>
class DisposableGuard;

}
