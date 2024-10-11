#pragma once

#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>

class Worker
{
public:
	Worker();
	~Worker();
	void Queue(std::function<void()> func);

private:
	void Loop();

private:
	std::mutex m_queueLock;
	std::queue<std::function<void()>> m_queue;
	std::condition_variable m_queueCondition;
	std::atomic<bool> m_running;

	std::thread m_thread;
};