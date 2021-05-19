#pragma once
#include <string_view>

namespace cold {

struct ComponentIds
{
	static constexpr std::string_view DefaultScheduler = "default_scheduler";
	static constexpr std::string_view PoolScheduler = "pool_scheduler";
	static constexpr std::string_view Network = "network";
};


}
