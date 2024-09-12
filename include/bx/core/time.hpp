#pragma once

#include <chrono>

//using Clock = std::chrono::high_resolution_clock;
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using TimeSpan = std::chrono::nanoseconds;

class Timer
{
public:
	void Start()
	{
		m_start = m_timer.now();
	}

	float Elapsed()
	{
		static const float toSeconds = 1.0f / 1000000.0f;
		return std::chrono::duration_cast<std::chrono::microseconds>(m_timer.now() - m_start).count() * toSeconds;
	}

private:
	Clock m_timer;
	TimePoint m_start;
};

class Time
{
public:
	static void Initialize();
	static void Update();

	static float GetDeltaTime();
	static float GetTime();
};