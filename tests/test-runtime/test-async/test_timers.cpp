#include "pch.h"
#include <cold/async/asynctimer.h>
#include <cold/async/task.h>
#include "runtimescopeguard.h"

using namespace cold;
using namespace cold::async;
using namespace testing;

using namespace std::chrono;
using namespace std::chrono_literals;


template<typename C>
decltype(auto) operator << (std::basic_ostream<C>& stream, const milliseconds& p)
{
	stream << p.count() << L"ms";
	return (stream);
}


namespace {

class Test_Async_Timers : public testing::Test
{
public:

private:
	const RuntimeScopeGuard m_runtimeGuard;
}; // namespace

} // namespace



/**
*/
TEST_F(Test_Async_Timers, DelayBasic)
{
	const auto doDelay = []<typename R, typename P>(duration<R, P> delay) -> Task<milliseconds>
	{
		const auto timePoint = system_clock::now();
		co_await delay;
		co_return duration_cast<milliseconds>(system_clock::now() - timePoint);
	};

	const auto actualDelay1 = async::waitResult(doDelay(20ms));
	ASSERT_THAT(actualDelay1.count(), Ge(18));

	const auto actualDelay2 = async::waitResult(doDelay(40ms));
	ASSERT_THAT(actualDelay2.count(), Ge(38));
}

/**
*/
TEST_F(Test_Async_Timers, Delay)
{
	constexpr size_t ThreadsCount = 20;
	constexpr size_t TasksPerThread = 20;
	constexpr size_t ExpectedCounter = ThreadsCount * TasksPerThread;

	// Task sources for all tasks from all threads.
	std::vector<TaskSource<>> taskSources;
	std::vector<Task<>> tasks;
	std::atomic_size_t counterEnter = 0;
	std::atomic_size_t counterReadyAwaiter = 0;
	std::atomic_size_t counterReadyAll = 0;
	std::mutex mutex;
	std::mutex mutex1;
	std::vector<std::thread> threads;

	std::atomic_uint64_t debugCounter{};


	for (size_t i = 0; i < ThreadsCount; ++i)
	{
		threads.emplace_back([this, &taskSources, &tasks, &counterEnter, &counterReadyAwaiter, &counterReadyAll, &mutex, &mutex1, TasksPerThread]()
		{
			std::vector<TaskSource<>> threadTasks;

			for (size_t j = 0; j < TasksPerThread; ++j)
			{
				Task<> task = [&threadTasks](std::atomic_size_t& counterEnter, std::atomic_size_t& counterReadyAwaiter, std::atomic_size_t& counterReadyAll) -> Task<>
				{
					++counterEnter;
					auto awaiter = threadTasks.emplace_back().getTask();

					co_await awaiter;
					++counterReadyAwaiter;

					co_await 5ms; // using small delay
					++counterReadyAll;

				}(counterEnter, counterReadyAwaiter, counterReadyAll);

				std::lock_guard lock {mutex1};
				tasks.emplace_back(std::move(task));
			}

			std::lock_guard lock(mutex);
			std::move(threadTasks.begin(), threadTasks.end(), std::back_inserter(taskSources));
		});
	}

	// wait while all threads perform their work - i.e. create task and push it into task collection.
	std::for_each(threads.begin(), threads.end(), [](std::thread& thread) {thread.join(); });

	// at this moment all tasks from all threads are live inside collection, but none of them if finished.
	ASSERT_THAT(taskSources.size(), Eq(ExpectedCounter));

	// allow tasks to finish their works. All tasks from this point are going to be finished.
	std::for_each(taskSources.begin(), taskSources.end(), [](TaskSource<>& t) {t.resolve(); });

	// await all pushed tasks.
	waitResult(whenAll(tasks));

	ASSERT_THAT(counterReadyAll, Eq(ExpectedCounter));
}


TEST_F(Test_Async_Timers, Create)
{
	auto task = []() -> Task<>
	{
		auto timer = async::createRuntimeTimer(50ms, 100ms);
		size_t ticks = 0;

		while (co_await timer->tick())
		{
			if (++ticks > 4)
			{
				timer->stop();
			}
		};
	}();

	async::waitResult(std::ref(task));
}

/**
*
*/
TEST_F(Test_Async_Timers, Simple)
{
	auto task = []() -> Task<bool>
	{
		const size_t TimersCount = 5;

		std::vector<Task<bool>> timerTasks;

		for (size_t i = 0; i < TimersCount; ++i)
		{
			timerTasks.emplace_back([](size_t index) -> Task<bool>
			{
				const auto RequestedInitialDelay = 20ms  + (10ms * index);

				const auto TickPeriod = 5ms;
				const size_t TicksCount = 10;

				const auto startTime = system_clock::now();

				auto timer = async::createRuntimeTimer(RequestedInitialDelay, TickPeriod);

				size_t tickCounter = 0;

				while (co_await timer->tick())
				{
					const auto actualDelay = (duration_cast<milliseconds>(system_clock::now() - startTime));

					if (tickCounter == 0)
					{
						const milliseconds DelayThreshold = 5ms;
						const milliseconds diff = (actualDelay - RequestedInitialDelay);

						if (std::abs(diff.count()) >= DelayThreshold.count())
						{
							co_return false;
						}
					}

					if (++tickCounter == TicksCount)
					{
						timer->stop();
					}
				}

				co_return true;

			}(i));

		}

		co_await whenAll(timerTasks);

		co_return std::all_of(timerTasks.begin(), timerTasks.end(), [](Task<bool>& task) { return task.result(); });
	}();


	const bool result = async::waitResult(std::ref(task));

	ASSERT_TRUE(result);
}


TEST_F(Test_Async_Timers, Stress)
{
	constexpr auto TimerRepeatTick = 30ms;
	constexpr auto TimersWaitTime = TimerRepeatTick * 5;
	constexpr size_t TimersCount = 1750;


	auto task = [TimerRepeatTick, TimersWaitTime, TimersCount]() -> async::Task<uint32_t>
	{
		std::list<async::Task<>> timerTasks;
		std::list<AsyncTimer::Ptr> timers;

		std::atomic_uint32_t counter {0ui32};

		std::mutex timersMutex;

		for (size_t i = 0; i < TimersCount; ++i)
		{
			timerTasks.emplace_back([&timers, &counter](size_t index, std::chrono::milliseconds repeatTick) -> async::Task<>
			{
				timers.emplace_front(async::createRuntimeTimer(0ms, repeatTick));

				auto timer = timers.front().get();

				while (co_await timer->tick())
				{
					counter.fetch_add(1);
				};
			}(i, TimerRepeatTick));
		}

		co_await async::createRuntimeTimer(TimersWaitTime + 20ms)->tick();

		for (auto& timer : timers)
		{
			timer->stop();
		}

		co_await async::whenAll(timerTasks);

		timers.clear();

		co_return counter.load();
	}();


	const uint32_t counter = async::waitResult(std::ref(task));

	ASSERT_GE(counter, (TimersWaitTime / TimerRepeatTick) * TimersCount);
}
