#pragma once
#include "Spinlock.h"

#include <queue>
#include <mutex>
#include <vector>
#include <atomic>
#include <functional>
#include <condition_variable>

#define MAX_THREADS 16U

class TaskDispatcher
{
public:
	DECL_STATIC_CLASS(TaskDispatcher);

	static bool init();
	static void release();

	//Excutes a task in a seperate thread
	static void execute(const std::function<void()>& task);
	//Makes sure that all queued up tasks have been completed
	static void waitForTasks();

	static FORCEINLINE bool isFinished()
	{
		return (s_CurrentFence <= s_FinishedFence.load());
	}

	static FORCEINLINE bool shouldRunWorker()
	{
		return s_RunWorkers;
	}

private:
	static bool poptask(std::function<void()>& task);
	static void poll();

	static void taskThread();

private:
	static std::vector<std::thread> s_Threads;

	static std::queue<std::function<void()>> s_TaskQueue;
	static Spinlock s_QueueLock;
	
	static std::mutex s_EventMutex;
	static std::condition_variable s_WakeCondition;
	
	static std::atomic<uint64_t> s_FinishedFence;
	static uint64_t s_CurrentFence;
	
	static bool s_RunWorkers;
};

