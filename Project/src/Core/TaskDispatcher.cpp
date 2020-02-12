#include "TaskDispatcher.h"

#include <algorithm>

std::vector<std::thread>			TaskDispatcher::s_Threads;
std::mutex							TaskDispatcher::s_EventMutex;
std::atomic<uint64_t>				TaskDispatcher::s_FinishedFence;
std::condition_variable				TaskDispatcher::s_WakeCondition;
std::queue<std::function<void()>>	TaskDispatcher::s_TaskQueue;
uint64_t							TaskDispatcher::s_CurrentFence = 0;
Spinlock							TaskDispatcher::s_QueueLock;
bool								TaskDispatcher::s_RunWorkers = true;

bool TaskDispatcher::init()
{
	const uint32_t numThreads = std::min(std::max(1U, std::thread::hardware_concurrency()), MAX_THREADS);

	LOG("TaskManager: Starting up %u threads", numThreads);

	s_RunWorkers = true;
	for (uint32_t i = 0; i < numThreads; i++)
	{
		s_Threads.emplace_back(taskThread);
	}

	return true;
}

void TaskDispatcher::release()
{
	s_RunWorkers = false;
	s_WakeCondition.notify_all();

	waitForTasks();
	for (std::thread& thread : s_Threads)
	{
		thread.join();
	}
}

void TaskDispatcher::execute(const std::function<void()>& task)
{
	s_CurrentFence++;

	{
		std::scoped_lock<Spinlock> lock(s_QueueLock);
		s_TaskQueue.push(task);
		s_WakeCondition.notify_one();
	}
}

void TaskDispatcher::waitForTasks()
{
	while (!isFinished())
	{
		poll();
	}
}

bool TaskDispatcher::poptask(std::function<void()>& task)
{
	std::scoped_lock<Spinlock> lock(s_QueueLock);
	if (!s_TaskQueue.empty())
	{
		task = s_TaskQueue.front();
		s_TaskQueue.pop();

		return true;
	}

	return false;
}

void TaskDispatcher::poll()
{
	s_WakeCondition.notify_one();
	std::this_thread::yield();
}

void TaskDispatcher::taskThread()
{
	while (shouldRunWorker())
	{
		std::function<void()> task;
		if (poptask(task))
		{
			task();
			s_FinishedFence.fetch_add(1);
		}
		else
		{
			std::unique_lock<std::mutex> lock(s_EventMutex);
			s_WakeCondition.wait(lock);
		}
	}

	LOG("Shutting down worker");
}
