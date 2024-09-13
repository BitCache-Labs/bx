#include "bx/engine/core/thread.hpp"

Worker::Worker()
	: m_queueLock()
	, m_queue()
	, m_queueCondition()
	, m_running(true)
	, m_thread(&Worker::Loop, this)
{}

Worker::~Worker()
{
	std::unique_lock<std::mutex> qlk(m_queueLock);
	m_running = false;

	qlk.unlock();
	m_queueCondition.notify_one();

	m_thread.join();
}

void Worker::Queue(std::function<void()> task)
{
	std::unique_lock<std::mutex> qlk(m_queueLock);
	m_queue.push(task);

	qlk.unlock();
	m_queueCondition.notify_one();
}

void Worker::Loop()
{
	std::function<void()> task;
	while (true)
	{
		std::unique_lock<std::mutex> qlk(m_queueLock);
		m_queueCondition.wait(qlk, [&]() { return !m_queue.empty() || !m_running; });

		if (!m_running)
			break;

		task = m_queue.front();
		m_queue.pop();

		task();
	}
}