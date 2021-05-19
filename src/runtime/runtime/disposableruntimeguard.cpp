#include "pch.h"
#include "cold/runtime/runtime.h"
#include "cold/runtime/disposableruntimeguard.h"
#include "cold/runtime/internal/runtimecomponent.h"
#include "cold/runtime/internal/runtimeallocator.h"
#include "cold/com/comclass.h"
#include "cold/memory/rtstack.h"
#include "cold/threading/critical_section.h"

namespace cold {

class DisposableRuntimeGuardComponent final : public RuntimeComponent, public Disposable
{
	COMCLASS_(RuntimeComponent, Disposable)

public:

	static constexpr std::string_view ComponentId = "disposables";

	static DisposableRuntimeGuardComponent& getThis()
	{
		ComPtr<> component = Runtime::instance().findComponent(ComponentId);
		DEBUG_CHECK(component)

		return component->as<DisposableRuntimeGuardComponent&>();
	}

	void registerInstance(WeakComPtr<> ptr)
	{
		lock_(m_mutex);
		DEBUG_CHECK(!m_isDisposed)
		DEBUG_CHECK(ptr)

		m_instances.emplace_back(std::move(ptr));
	}

	void unregisterInstance(IWeakReference* targetPtr)
	{
		if (!targetPtr)
		{
			return;
		}

		lock_(m_mutex);

		auto iter = std::find_if(m_instances.begin(), m_instances.end(), [targetPtr](const WeakComPtr<>& ptr) { return ptr.get() == targetPtr; });
		if (iter != m_instances.end())
		{
			m_instances.erase(iter);
		}
	}


private:

	bool componentHasWorks() override
	{
		lock_(m_mutex);
		return !m_instances.empty();
	}


	void dispose() override
	{
		rtstack();

		std::vector<ComPtr<>, RtStackStdAllocator<ComPtr<>>> instances;

		{
			lock_(m_mutex);
			if (const bool alreadyDisposed = m_isDisposed.exchange(true); alreadyDisposed)
			{
				return;
			}

			instances.reserve(m_instances.size());

			for (auto iter = m_instances.begin(); iter != m_instances.end();)
			{
				if (auto instance = iter->acquire(); instance)
				{
					instances.emplace_back(std::move(instance));
					++iter;
				}
				else
				{
					iter = m_instances.erase(iter);
				}
			}
		}

		for (ComPtr<>& instance : instances)
		{
			if (Disposable* const disposable = instance->as<Disposable*>(); disposable)
			{
				try
				{
					disposable->dispose();
				}
				catch (const std::exception& exc)
				{
					LOG_error_("Unexpected exception while disposing:{0}", exc.what())
				}
			}
		}
	}

	std::list<WeakComPtr<>/*, RuntimeStdAllocator<WeakComPtr<>>*/> m_instances;
	threading::CriticalSection m_mutex;
	std::atomic_bool m_isDisposed = false;
};


//-----------------------------------------------------------------------------
DisposableRuntimeGuard::DisposableRuntimeGuard() = default;

DisposableRuntimeGuard::DisposableRuntimeGuard(IRefCounted& disposable)
{
	if (disposable.is<Disposable>())
	{
		WeakComPtr<IRefCounted> weakPtr{com::Acquire{&disposable}};
		m_handle = weakPtr.get();
		DEBUG_CHECK(m_handle)
		DisposableRuntimeGuardComponent::getThis().registerInstance(std::move(weakPtr));
	}
}

DisposableRuntimeGuard::DisposableRuntimeGuard(DisposableRuntimeGuard&& other) noexcept : m_handle(other.m_handle)
{
	this->dispose();
}

DisposableRuntimeGuard::~DisposableRuntimeGuard()
{
	DisposableRuntimeGuardComponent::getThis().unregisterInstance(reinterpret_cast<cold::IWeakReference*>(m_handle));
}

DisposableRuntimeGuard& DisposableRuntimeGuard::operator = (DisposableRuntimeGuard&& other) noexcept
{
	this->dispose();
	DEBUG_CHECK(!m_handle);
	std::swap(m_handle, other.m_handle);

	return *this;
}

void DisposableRuntimeGuard::dispose()
{
	IWeakReference* const ptr = reinterpret_cast<cold::IWeakReference*>(m_handle);
	m_handle = nullptr;
	DisposableRuntimeGuardComponent::getThis().unregisterInstance(ptr);
}


namespace {

[[maybe_unused]] const auto registration = RuntimeComponent::registerComponent(DisposableRuntimeGuardComponent::ComponentId, [](uv_loop_t*, std::string_view, void*) noexcept -> RuntimeComponent::Ptr
{
	return com::createInstanceSingleton<DisposableRuntimeGuardComponent, RuntimeComponent>();
}
, nullptr);

} // namespace

} // namespace cold

