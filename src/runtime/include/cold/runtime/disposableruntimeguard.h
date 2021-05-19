#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/com/comptr.h>
#include <cold/utils/disposable.h>
#include <cold/utils/preprocessor.h>

namespace cold {

class RUNTIME_EXPORT DisposableRuntimeGuard final: public Disposable
{
public:

	DisposableRuntimeGuard(IRefCounted& disposable);

	DisposableRuntimeGuard();

	DisposableRuntimeGuard(const DisposableRuntimeGuard&) = delete;

	DisposableRuntimeGuard(DisposableRuntimeGuard&&) noexcept;

	~DisposableRuntimeGuard();

	DisposableRuntimeGuard& operator = (const DisposableRuntimeGuard&) = delete;

	DisposableRuntimeGuard& operator = (DisposableRuntimeGuard&&) noexcept;

	void dispose() override;

private:

	void* m_handle = nullptr;
};

}


#define rtdisposable(disposable) const cold::DisposableRuntimeGuard ANONYMOUS_VARIABLE_NAME(rtDisposableGuard__) {(disposable)}
