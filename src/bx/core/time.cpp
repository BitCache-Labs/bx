#include "bx/engine/core/time.hpp"

static Timer s_timer;
static float s_time = 0;
static float s_deltaTime = 0;

void Time::Initialize()
{
	s_timer.Start();
}

void Time::Update()
{
	s_deltaTime = s_timer.Elapsed();
	s_timer.Start();

	s_time += s_deltaTime;
}

float Time::GetDeltaTime()
{
	return s_deltaTime;
}

float Time::GetTime()
{
	return s_time;
}