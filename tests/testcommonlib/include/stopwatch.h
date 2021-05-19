#pragma once
#include <chrono>

class StopWatch
{
public:
	StopWatch();

	std::chrono::milliseconds timePassed() const;

	size_t ms() const;

private:

	const std::chrono::system_clock::time_point m_timePoint;
};

