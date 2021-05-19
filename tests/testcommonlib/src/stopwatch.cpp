#include "pch.h"
#include "stopwatch.h"


StopWatch::StopWatch(): m_timePoint(std::chrono::system_clock::now())
{}

std::chrono::milliseconds StopWatch::timePassed() const
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(std::chrono::system_clock::now() - m_timePoint);
}

size_t StopWatch::ms() const
{
	return this->timePassed().count();
}
