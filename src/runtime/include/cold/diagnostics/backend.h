#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/com/interface.h>

#include <memory>


namespace cold::diagnostics {


//struct ABSTRACT_TYPE BackendGuard
//{
//	RUNTIME_EXPORT static std::unique_ptr<BackendGuard> create();
//
//	virtual ~BackendGuard() = default;
//};


/// <summary>
///
/// </summary>
struct ABSTRACT_TYPE Backend
{
	RUNTIME_EXPORT static std::unique_ptr<Backend> create();

	RUNTIME_EXPORT static Backend& instance();

	RUNTIME_EXPORT static bool exists();

	virtual ~Backend() = default;
};


}
