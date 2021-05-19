#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/com/ianything.h>
#include <cold/com/comptr.h>
#include <cold/utils/functor.h>
#include <cold/utils/disposable.h>

#include <uv.h>


#ifdef RUNTIME_BUILD
#include <vector>
#endif


namespace cold {


struct [[nodiscard]] RUNTIME_EXPORT RuntimeComponentRegistrationHandle
{
	RuntimeComponentRegistrationHandle(void*);

	~RuntimeComponentRegistrationHandle();

	RuntimeComponentRegistrationHandle(RuntimeComponentRegistrationHandle&) = delete;

	RuntimeComponentRegistrationHandle(RuntimeComponentRegistrationHandle&&);

	RuntimeComponentRegistrationHandle& operator = (const RuntimeComponentRegistrationHandle&) = delete;

	RuntimeComponentRegistrationHandle& operator = (RuntimeComponentRegistrationHandle&&);

private:

	void* handle = nullptr;
};


struct ABSTRACT_TYPE RuntimeComponent : IRefCounted
{
	using Ptr = ComPtr<RuntimeComponent>;
	using Factory = RuntimeComponent::Ptr (*) (uv_loop_t*, std::string_view, void*) noexcept;
	static RuntimeComponentRegistrationHandle registerComponent(std::string_view id, RuntimeComponent::Factory factory, void* data);

	virtual bool componentHasWorks() = 0;
};


#ifdef RUNTIME_BUILD

struct ComponentEntry
{
	const std::string_view id;
	const RuntimeComponent::Ptr component;
};

std::vector<ComponentEntry> createRuntimeComponents(uv_loop_t* uv);
#endif

}
