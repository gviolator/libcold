#pragma once

#include <thread>


struct RuntimeScopeGuard
{
	std::thread thread;

	RuntimeScopeGuard();

	~RuntimeScopeGuard();

	void reset();
};
