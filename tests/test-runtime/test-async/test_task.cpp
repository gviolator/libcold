#include "pch.h"
#include <cold/async/task.h>
#include <cold/utils/scopeguard.h>
#include <cold/runtime/runtime.h>
#include <cold/threading/event.h>
#include <cold/threading/barrier.h>

#include "runtimescopeguard.h"


using namespace cold;
using namespace cold::async;
using namespace testing;

using namespace std::chrono_literals;

TEST(Test_Task, StatelessByDefault)
{
	Task<> taskVoid;
	ASSERT_FALSE(taskVoid);

	Task<int> taskNonVoid;
	ASSERT_FALSE(taskNonVoid);

}


TEST(Test_Task, Copy)
{
	static_assert(!std::is_copy_constructible_v<TaskSource<>>);
	static_assert(!std::is_copy_constructible_v<TaskSource<int>>);
	static_assert(!std::is_copy_constructible_v<Task<>>);
	static_assert(!std::is_copy_constructible_v<Task<int>>);
}


TEST(Test_Task, TaskSourceMoveConstruct)
{
	static_assert(std::is_move_constructible_v<TaskSource<>>);
	static_assert(std::is_move_constructible_v<TaskSource<int>>);

	TaskSource<> taskVoidSource;
	ASSERT_TRUE(taskVoidSource);

	TaskSource<> taskVoidSource2 = std::move(taskVoidSource);
	
	ASSERT_TRUE(taskVoidSource2 );
	ASSERT_FALSE(taskVoidSource);


	TaskSource<int> taskNonVoidSource;
	ASSERT_TRUE(taskNonVoidSource);

	auto taskNonVoidSource2 = std::move(taskNonVoidSource);
	
	ASSERT_TRUE(taskNonVoidSource2);
	ASSERT_FALSE(taskNonVoidSource);
}




/// <summary>
///Test:
/// TaskSource:
///  > emplaceResult;
///  > setException;
///  > setReady;
///
/// Task:
///  > ready();
///  > result();
///  > rethrow();
///  > exceptionPtr();
//

///
/// 1. Initialize task sources.
/// 2. Get tasks from sources.
/// 3. Run async operation to populate task sources with result: chooser random result or exception.
/// 4. Run async operations to wait while all tasks are ready.
/// 5. For all tasks check:
///  > task ready;
///  > task has value or exception;
///  > for exceptional case check that 'result' is throw, 'rethrow' is throw, check exception type and exception data.
/// </summary>
/// <param name=""></param>
/// <param name=""></param>
/// <returns></returns>
TEST(Test_Task, Resolve)
{
	std::vector<TaskSource<size_t>> taskSources;
	std::vector<TaskSource<>> voidTaskSources;

	for (size_t i = 0; i < 2000; ++i)
	{
		taskSources.emplace_back();
		voidTaskSources.emplace_back();
	}


	std::vector<Task<size_t>> tasks;
	tasks.reserve(taskSources.size());

	std::vector<Task<>> voidTasks;
	voidTasks.reserve(voidTaskSources.size());

	std::transform(taskSources.begin(), taskSources.end(), std::back_inserter(tasks), [](TaskSource<size_t>& source)
	{
		return source.getTask();
	});

	std::transform(voidTaskSources.begin(), voidTaskSources.end(), std::back_inserter(voidTasks), [](TaskSource<>& source)
	{
		return source.getTask();
	});


	for (size_t index = 0; index < tasks.size(); ++index)
	{
		Task<size_t>& task = tasks[index];
		Task<>& voidTask = voidTasks[index];

		ASSERT_TRUE(task);
		ASSERT_TRUE(voidTask);

		ASSERT_FALSE(task.ready());
		ASSERT_FALSE(voidTask.ready());
	}

	const auto getExceptionMessage = [](size_t index) -> std::string
	{
		return format("Exception: %1", index);
	};


	std::future<void> f1 = std::async(std::launch::async,[&taskSources, &voidTaskSources, &getExceptionMessage]
	{
		std::random_device rd;

		auto randomTrueValue = [re = std::default_random_engine (rd()), randomizer = std::uniform_int_distribution<>{0, 1}]() mutable -> bool
		{
			return randomizer(re) != 0;
		};


		for (size_t index = 0; index < taskSources.size(); ++index)
		{
			if (randomTrueValue())
			{
				taskSources[index].resolve(index);
			}
			else
			{
				taskSources[index].reject(std::exception(getExceptionMessage(index).c_str()));
			}

			if (randomTrueValue())
			{
				voidTaskSources[index].resolve();
			}
			else
			{
				voidTaskSources[index].reject(std::exception(getExceptionMessage(index).c_str()));
			}
		}
	});

	std::future<void> f2 = std::async(std::launch::async, [&tasks]
	{
		while (!std::all_of(tasks.begin(), tasks.end(), [](Task<size_t>& task)
		{
			return task.ready();
		}));
	});

	std::future<void> f3 = std::async(std::launch::async, [&voidTasks]
	{
		while (!std::all_of(voidTasks.begin(), voidTasks.end(), [](Task<>& task)
		{
			return task.ready();
		}));
	});

	f1.wait();
	f2.wait();
	f3.wait();

	for (size_t index = 0; index < tasks.size(); ++index)
	{
		Task<size_t>& task = tasks[index];
		ASSERT_TRUE(task.ready());

		if (task.exception())
		{
			ASSERT_THROW(task.result(), std::exception);
			ASSERT_THROW(task.rethrow(), std::exception);

			try
			{
				task.rethrow();
			}
			catch (const std::exception& exception)
			{
				ASSERT_EQ(getExceptionMessage(index), exception.what());
			}
		}
		else
		{
			ASSERT_NO_THROW(task.rethrow());

			ASSERT_EQ(index, task.result());
		}

		Task<>& voidTask = voidTasks[index];
		ASSERT_TRUE(voidTask.ready());

		if (voidTask.exception())
		{
			ASSERT_THROW(voidTask.result(), std::exception);
			ASSERT_THROW(voidTask.rethrow(), std::exception);

			try
			{
				voidTask.rethrow();
			}
			catch (const std::exception& exception)
			{
				ASSERT_EQ(getExceptionMessage(index), exception.what());
			}
		}
		else
		{
			ASSERT_NO_THROW(voidTask.rethrow());

			ASSERT_NO_THROW(
				voidTask.result()
			);
		}
	}
}




/// <summary>
///
/// </summary>
/// <param name=""></param>
/// <param name=""></param>
/// <returns></returns>
TEST(Test_Task, Wait)
{
	{ // wait without timeout
		TaskSource<std::thread::id> taskSource;

		auto task = taskSource.getTask();

		const auto threadSleepTime = 70ms;

		threading::Event signal;

		std::thread t([taskSource = std::move(taskSource), threadSleepTime, &signal]() mutable
		{
			std::this_thread::sleep_for(threadSleepTime);

			taskSource.resolve(std::this_thread::get_id());

			signal.wait();
		});

		const auto time = std::chrono::system_clock::now();

		ASSERT_TRUE(async::wait(task));

		ASSERT_TRUE(task.ready());
		ASSERT_EQ(t.get_id(), task.result());

		const auto timePassed = std::chrono::system_clock::now() - time;

		ASSERT_TRUE(timePassed >= threadSleepTime);

		signal.set();
		t.join();
	}

	{ // wait with timeout
		TaskSource<std::thread::id> taskSource;

		auto task = taskSource.getTask();

		const auto taskWaitTime = 50ms;
		const auto threadSleepTime = taskWaitTime * 2;

		threading::Event signal;

		std::thread t([taskSource = std::move(taskSource), threadSleepTime, &signal]() mutable
		{
			std::this_thread::sleep_for(threadSleepTime);

			signal.wait();

			taskSource.resolve(std::this_thread::get_id());
		});

		ASSERT_FALSE(async::wait(task, taskWaitTime));
		ASSERT_FALSE(task.ready());

		signal.set();

		ASSERT_TRUE(async::wait(task));

		ASSERT_EQ(t.get_id(), task.result());

		t.join();
	}
};


TEST(Test_Task, AwaiterNonVoid)
{
	RuntimeScopeGuard runtimeGuard;

	TaskSource<int> taskSource;

	const int Value = 10;

	auto awaiter = [](Task<int> task) -> Task<int>
	{
		const auto value = co_await std::move(task);
		co_return (value * 2);
	} (taskSource.getTask());

	std::thread([&taskSource, Value]
	{
		std::this_thread::sleep_for(50ms);
		taskSource.resolve(Value);
	})
	.join();

	async::wait(awaiter);

	EXPECT_THAT(awaiter.result(), Eq(Value * 2));
}


TEST(Test_Task, AwaiterVoid)
{
	RuntimeScopeGuard runtimeGuard;

	TaskSource<> taskSource;

	auto awaiter = [](Task<> task) -> Task<>
	{
		co_await std::move(task);
		co_return ;
	} (taskSource.getTask());

	std::thread([&taskSource]
	{
		std::this_thread::sleep_for(50ms);
		taskSource.resolve();
	})
	.join();

	async::wait(awaiter);

	ASSERT_NO_THROW(awaiter.result());
}


TEST(Test_Task, AwaiterReject)
{
	RuntimeScopeGuard runtimeGuard;

	TaskSource<> taskSource;

	auto awaiter = [](Task<> task) -> Task<int>
	{
		co_await std::move(task);
		co_return 0;
	} (taskSource.getTask());

	std::thread([&taskSource]
	{
		std::this_thread::sleep_for(50ms);
		taskSource.reject(std::exception("TestFailure"));
	})
	.join();

	async::wait(awaiter);

	ASSERT_TRUE(awaiter.rejected());
	ASSERT_THROW(awaiter.result(), std::exception);
}


TEST(Test_Task, WhenAny)
{
	RuntimeScopeGuard runtimeGuard;

	using TaskSources = std::vector<TaskSource<>>;

	TaskSources taskSources(10);
	std::vector<Task<>> tasks;
	std::transform(taskSources.begin(), taskSources.end(), std::back_inserter(tasks), [](TaskSource<>& ts) { return ts.getTask().detach(); });

	Task<bool> awaiter = async::whenAny(tasks);

	std::thread thread([](TaskSources taskSources)
	{
	
		std::this_thread::sleep_for(10ms);

		for (TaskSource<>& ts : taskSources)
		{
			ts.resolve();
		}

	}, std::move(taskSources));

	async::wait(awaiter);
	thread.join();


	LOG_debug_("done")
}



TEST(Test_Task, WhenAll)
{
	RuntimeScopeGuard runtimeGuard;

	constexpr size_t ThreadsCount = 10;
	constexpr size_t TasksCount = 1000;
	constexpr size_t TasksPerThread = TasksCount / ThreadsCount;

	static_assert(TasksCount % ThreadsCount == 0);

	using TaskSources = std::vector<TaskSource<int>>;

	TaskSources taskSources(TasksCount);
	std::vector<Task<int>> tasks;
	std::transform(taskSources.begin(), taskSources.end(), std::back_inserter(tasks), [](auto& ts) { return ts.getTask().detach(); });

	Task<bool> awaiter = async::whenAll(tasks);

	const auto timePoint = std::chrono::system_clock::now();

	std::vector<std::thread> threads(ThreadsCount);

	threading::Barrier barrier{ThreadsCount};

	for (std::thread& thread : threads)
	{
		TaskSources threadTaskSources(
			std::make_move_iterator(taskSources.begin()),
			std::make_move_iterator(taskSources.begin() + TasksPerThread));

		taskSources.erase(taskSources.begin(), taskSources.begin() + TasksPerThread);

		thread = std::thread([](TaskSources threadTaskSources, threading::Barrier& barrier)
		{
			barrier.enter();
			for (auto& ts : threadTaskSources)
			{
				ts.resolve(10);
			}
		}, std::move(threadTaskSources), std::ref(barrier));
	}

	async::wait(awaiter);
	std::for_each(threads.begin(), threads.end(), [](std::thread& t){ t.join(); });
}


TEST(Test_Task, WhenAny_Old)
{
	RuntimeScopeGuard runtimeGuard;

	//std::vector<TaskSource<>> taskSources(2000);
	std::vector<TaskSource<>> taskSources(2);

	std::vector<Task<>> tasks(taskSources.size());

	std::transform(taskSources.begin(), taskSources.end(), tasks.begin(), [](TaskSource<>& taskSource)
	{
		auto task = taskSource.getTask();
		task.detach();

		return task;
	});


	std::vector<size_t> readyIndices;

	const auto threadSleepTime = 70ms;

	const size_t expectReadyTasksCount = 1;

	threading::Event signal;

	std::thread theThread([taskSources = std::move(taskSources), &readyIndices, threadSleepTime, &signal, expectReadyTasksCount]() mutable
	{
		std::this_thread::sleep_for(threadSleepTime);

		std::random_device rd{};
		std::default_random_engine re(rd());
		std::uniform_int_distribution<size_t> randomizer(0, taskSources.size() - 1);

		auto nextIndex = [&randomizer, &re, &readyIndices]() mutable
		{
			size_t index ;

			do
			{
				index = randomizer(re);
			}
			while (std::find(readyIndices.begin(), readyIndices.end(), index) != readyIndices.end());

			return index;
		};


		for (size_t i = 0; i < expectReadyTasksCount; ++i)
		{
			const size_t index = nextIndex();

			auto& taskSource = taskSources[index];

			if (!taskSource.ready())
			{
				taskSource.resolve();
				readyIndices.push_back(index);
			}
		}

		signal.wait();
	});

	const auto startTime = std::chrono::system_clock::now();

	for (size_t repeat = 0; repeat < 10; ++repeat)
	{
		auto awaiter = async::whenAny(tasks);

		async::wait(awaiter);

		if (repeat == 0)
		{
			const auto timePassed = std::chrono::system_clock::now() - startTime;

			ASSERT_GE(timePassed, threadSleepTime);
		}
	}


	for (size_t index = 0; index < tasks.size(); ++index)
	{
		auto& task = tasks[index];

		const bool taskShouldBeReady = std::find(readyIndices.begin(), readyIndices.end(), index) != readyIndices.end();

		if (taskShouldBeReady)
		{
			ASSERT_TRUE(task.ready());
		}
		else
		{
			ASSERT_FALSE(task.ready());
		}
	}

	signal.set();
	theThread.join();

	ASSERT_EQ(expectReadyTasksCount, readyIndices.size());
};

/// <summary>
///
/// </summary>

TEST(Test_Task, WhenAll_Old)
{
#if 0
	RuntimeScopeGuard runtimeGuard;

	constexpr size_t ThreadsCount = 6;
	constexpr size_t TasksPerThread = 2000;
	constexpr auto ThreadSleepTime = 40ms;

	std::vector<TaskSource<>> taskSources(ThreadsCount * TasksPerThread);

	std::vector<Task<>> tasks(taskSources.size());

	std::transform(taskSources.begin(), taskSources.end(), tasks.begin(), [](TaskSource<>& taskSource)
	{
		return taskSource.getTask();
	});


	std::vector<std::thread> threads;

	for (size_t i = 0; i < ThreadsCount; ++i)
	{

		std::thread thread([taskSources = std::move(threadSources), ThreadSleepTime]() mutable
		{
			std::this_thread::sleep_for(ThreadSleepTime);

			const size_t lastIndex = taskSources.size() - 1;

			for (size_t i = 0; i < lastIndex; ++i)
			{
				taskSources[i].resolve();
			}

			std::this_thread::sleep_for(ThreadSleepTime);
			// last source should automatically set exception within its destructor (because result was not set explicitly).
		});

		threads.emplace_back(std::move(thread));
	}

	const auto startTime = std::chrono::system_clock::now();

	for (size_t repeat = 0; repeat < 5; ++repeat)
	{
		auto awaiter = async::whenAll(tasks);

		async::wait(awaiter);

		if (repeat == 0)
		{
			const auto timePassed = std::chrono::system_clock::now() - startTime;

			const auto expectedWaitTime = ThreadSleepTime + ThreadSleepTime;

			ASSERT_GE(timePassed, expectedWaitTime);
		}
	}

	std::for_each(threads.begin(), threads.end(), [](std::thread& thread)
	{
		thread.join();
	});


	for (size_t index = 0; index < tasks.size(); ++index)
	{
		auto& task = tasks[index];

		ASSERT_TRUE(task.ready());

		const bool expectException = ((index + 1) % TasksPerThread == 0);

		if (expectException)
		{
			ASSERT_THROW(task.rethrow(), std::exception);
		}
		else
		{
			ASSERT_NO_THROW(task.rethrow());
		}
	}

	LOG_debug_("done")
#endif
}

#if 0

/// <summary>
/// 
/// </summary>
TEST(Async_Tasks, WaitOnSignleThreadScheduler)
{
	auto app = createSingleThreadSchedulerApp();

	auto task = async::run_([]() -> Task<>
	{
		std::vector<TaskSource<>> taskSources;
		std::vector<Task<>> tasks;

		for (size_t i = 0; i < 10; ++i)
		{
			tasks.emplace_back([](Task<> awaiter) -> Task<>
			{
				co_await awaiter;
				co_await Delay(2ms);
			}(taskSources.emplace_back().getTask()));
		}

		auto waitAllTask = async::whenAll(tasks);

		for (auto& taskSource : taskSources)
		{
			taskSource.setReady();
		}

		co_await waitAllTask;

	}, app->schedulerUid());


	async::waitResult(std::ref(task));
}
#endif

namespace {

struct DestructibleObject
{
	virtual ~DestructibleObject() = default;
};


template<typename F>
struct DestructibleObjectAction : DestructibleObject
{
	F f;

	DestructibleObjectAction(F f_)
		: f(std::move(f_))
	{}

	~DestructibleObjectAction()
	{
		f();
	}

};


template<typename F>
std::shared_ptr<DestructibleObject> makeSharedDestructible(F&& f)
{
	return std::make_shared<DestructibleObjectAction<F>>(std::forward<F>(f));
}

}


TEST(Test_Task, ResultDestruction)
{
	RuntimeScopeGuard runtimeGuard;

	using DestructibleObjectPtr = std::shared_ptr<DestructibleObject>;

	auto task = []() -> Task<bool> {
		std::atomic_bool destructed = false;

		{
			auto innerTask = async::run([](std::atomic_bool& flag) -> Task<DestructibleObjectPtr> {
				co_return makeSharedDestructible([&flag]
				{
					flag = true;
				});
			}
			, {}, std::ref(destructed));


			[[maybe_unused]] auto temp = co_await innerTask;
		}

		co_return destructed.load();
	}();

	const bool success = async::waitResult(std::ref(task));

	ASSERT_TRUE(success);
}



TEST(Test_Task, ReadyCallbackCalled)
{
	TaskSource<> taskSource;

	bool callbackCalled = false;

	{
		core::CoreTask*coreTask = core::getCoreTask(taskSource);

		coreTask->setReadyCallback([](void* data1, void*) noexcept {
			*reinterpret_cast<bool*>(data1) = true;
		}, &callbackCalled);
	}

	taskSource.resolve();

	ASSERT_TRUE(callbackCalled);
}


#if 0

namespace {

constexpr int StateNotModified = 0;
constexpr int StateModified = 1;
constexpr int StateInvalid = 2;
constexpr int StateFailure = 3;

}


TEST(Async_Tasks, ReadyCallbackNeverCalledAfterItWasReseted)
{
	using State = std::tuple<int, size_t, void*>;

	constexpr size_t Count = 200;

	std::vector<Task<>> tasks;
	std::vector<State> state(Count);

	for (size_t i = 0; i < state.size(); ++i)
	{
		TaskSource taskSource;

		state[i] = State(StateNotModified, i, &state);

		async_internal::getCoreState(taskSource)->setReadyCallback([](void* data, void*) noexcept
		{
			auto& [flag, index, ptr] = *reinterpret_cast<State*>(data);

			std::this_thread::sleep_for(5ms);

			if (flag == StateInvalid)
			{ // callback must be never be called after it was reseted.
				flag = StateFailure;
			}
		}, &state[i]);


		tasks.emplace_back(run([task = taskSource.getTask(), index = i, &state]() mutable -> Task<>
		{
			async_internal::getCoreState(task)->setReadyCallback(nullptr, nullptr); // expect that after this line is called, callback must be never called.

			std::get<0>(state[index]) = StateInvalid;

			co_await task;

		}, {}));


		tasks.emplace_back(run([taskSource = std::move(taskSource)]() mutable
		{
			taskSource.setReady();
		}, {}));
	}

	async::waitResult(async::whenAll(tasks));

	const bool hasFailureState = std::any_of(state.begin(), state.end(), [](const State& state) { return std::get<0>(state) == StateFailure; });

	ASSERT_FALSE(hasFailureState);
}
#endif




