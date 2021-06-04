#include "pch.h"
#include "cold/diagnostics/runtimecheck.h"
#include "cold/runtime/runtime.h"
#include "cold/runtime/internal/runtimeinternal.h"
#include "cold/runtime/internal/runtimeallocator.h"
#include "cold/runtime/internal/uvutils.h"
#include "cold/runtime/internal/runtimecomponent.h"
#include "cold/utils/disposable.h"
#include "cold/memory/rtstack.h"
#include "cold/com/comclass.h"

#include "componentids.h"


using namespace cold::async;
using namespace cold::cold_literals;

namespace cold {

//-----------------------------------------------------------------------------

namespace {

struct RuntimeState
{
	Runtime* runtime = nullptr;
	Allocator::Ptr runtimePoolAllocator;
	AllocatorMemoryResource runtimePoolMemoryResource;
	
	RuntimeState();
};

enum class RuntimeStateAccess
{
	Initialize,
	Reset,
	Acquire
};


RuntimeState* getRuntimeState(RuntimeStateAccess what = RuntimeStateAccess::Acquire)
{
	static std::optional<RuntimeState> runtimeState;

	if (what == RuntimeStateAccess::Initialize)
	{
		DEBUG_CHECK(!runtimeState, "Runtime state already initialized")
		return &runtimeState.emplace();
	}
	else if (what == RuntimeStateAccess::Reset)
	{
		DEBUG_CHECK(runtimeState, "Runtime state expected to be initialized")
		DEBUG_CHECK(runtimeState->runtimePoolAllocator->refsCount() == 1)

		runtimeState.reset();
		return nullptr;
	}

	return runtimeState ? &(*runtimeState) : nullptr;
}


inline RuntimeState& requireRuntimeState()
{
	auto runtimeState = getRuntimeState();
	DEBUG_CHECK(runtimeState, "Runtime does not created")

	return *runtimeState;
}


struct RuntimeStateKeeper
{
	RuntimeStateKeeper(Runtime* runtime_, uv_loop_t* uv)
	{
		getRuntimeState(RuntimeStateAccess::Initialize)->runtime = runtime_;

		_putenv_s("UV_THREADPOOL_SIZE", "10");
		UV_RUNTIME_CHECK(uv_loop_init(uv))
		uv_loop_set_data(uv, runtime_);
	}

	~RuntimeStateKeeper()
	{
		getRuntimeState(RuntimeStateAccess::Reset);
	}
};


RuntimeState::RuntimeState(): runtimePoolMemoryResource([]() -> Allocator*
{
	return requireRuntimeState().runtimePoolAllocator.get();
})
{
	runtimePoolAllocator = createPoolAllocator({
		.concurrent = true,
		.granularity = PoolAllocatorConfig::DefaultGranularity
	});
}

} // namespace


//RuntimeInternal::RuntimeInternal(): m_runtimeThreadId(std::this_thread::get_id())
//{}


/**
*/
class RuntimeImpl : public Runtime
{
	CLASS_INFO(
		CLASS_BASE(Runtime)
	)

	SINGLETON_MEMOP(RuntimeImpl)

public:

	RuntimeImpl()
		: m_runtimeStateKeeper(this, &m_uv)
		, m_threadId(std::this_thread::get_id())
		, m_components(createRuntimeComponents(&m_uv))
	{
	}

	~RuntimeImpl()
	{
		const auto hasReferences = [this]() -> bool
		{
			return std::any_of(m_components.begin(), m_components.end(), [](const ComponentEntry& entry) {
				return entry.component->componentHasWorks() || entry.component->refsCount() > 1;
			});
		};

		for (const auto& entry : m_components)
		{
			if (Disposable* const disposable = entry.component->as<Disposable*>())
			{
				disposable->dispose();
			}
		}

		while (hasReferences())
		{
			uv_run(&m_uv, UV_RUN_NOWAIT);
		}

		m_components.clear();

		while (uv_loop_alive(&m_uv))
		{
			if (uv_run(&m_uv, UV_RUN_NOWAIT) == 0)
			{
				break;
			}
		}

		UV_RUNTIME_CHECK(uv_loop_close(&m_uv))
	}

	void run(RunMode mode) override
	{
		uv_run(&m_uv, UV_RUN_DEFAULT);
	}

	void stop() override
	{
		scheduler()->schedule([](void* ptr, void*) noexcept
		{
			RuntimeImpl& this_ = *reinterpret_cast<RuntimeImpl*>(ptr);
			uv_stop(&this_.m_uv);
		}, this);
	}

	Scheduler::Ptr scheduler()
	{
		ComPtr<> component = findComponent(ComponentIds::DefaultScheduler);
		DEBUG_CHECK(component)

		return component;
	}

	Scheduler::Ptr poolScheduler() const override
	{
		//return m_poolScheduler;
		ComPtr<> component = findComponent(ComponentIds::PoolScheduler);
		DEBUG_CHECK(component)

		return component;
	}

	ComPtr<> findComponent(std::string_view id) const
	{
		if (const auto entry = std::find_if(m_components.begin(), m_components.end(), [id](const ComponentEntry& c){ return c.id == id; }); entry != m_components.end())
		{
			return entry->component;
		}

		return ComPtr{};
	}

private:

	uv_loop_t* uv()
	{
		return &m_uv;
	}

	const RuntimeStateKeeper m_runtimeStateKeeper;
	const std::thread::id m_threadId;

	uv_loop_t m_uv;
	std::vector<ComponentEntry> m_components;
	
	friend struct RuntimeInternal;
	friend struct Runtime;
};


//-----------------------------------------------------------------------------

std::unique_ptr<Runtime> Runtime::create()
{
	return std::make_unique<RuntimeImpl>();
}


bool Runtime::exists()
{
	auto* const rtState = getRuntimeState();
	return rtState && rtState->runtime ;
}


Runtime& Runtime::instance()
{
	RuntimeState* const rtState = getRuntimeState();
	DEBUG_CHECK(rtState && rtState->runtime, "Runtime instance not exists")
	return *rtState->runtime;
}


bool Runtime::isRuntimeThread()
{
	return static_cast<RuntimeImpl&>(Runtime::instance()).m_threadId == std::this_thread::get_id();
}


Scheduler::Ptr RuntimeInternal::scheduler()
{
	return static_cast<RuntimeImpl&>(Runtime::instance()).scheduler();
}


uv_loop_t* RuntimeInternal::uv()
{
	DEBUG_CHECK(Runtime::isRuntimeThread())
	return &static_cast<RuntimeImpl&>(Runtime::instance()).m_uv;
}



void* runtimePoolAllocate(size_t size) noexcept
{
	return requireRuntimeState().runtimePoolAllocator->alloc(size);
}


void runtimePoolFree(void* ptr, std::optional<size_t> size) noexcept
{
	return requireRuntimeState().runtimePoolAllocator->free(ptr, size);
}

Allocator::Ptr& runtimePoolAllocator()
{
	return requireRuntimeState().runtimePoolAllocator;
}

std::pmr::memory_resource* runtimePoolMemoryResource()
{
	return &requireRuntimeState().runtimePoolMemoryResource;
}

//-----------------------------------------------------------------------------

//void* RuntimePoolMemoryResource::do_allocate(size_t size, size_t)
//{
//	return requireRuntimeState().runtimePoolAllocator->alloc(size);
//}
//
//void RuntimePoolMemoryResource::do_deallocate(void* ptr, size_t size, size_t)
//{
//	return requireRuntimeState().runtimePoolAllocator->free(ptr, size);
//}
//
//bool RuntimePoolMemoryResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept
//{
//	return static_cast<const std::pmr::memory_resource*>(this) == &other;
//}

}


