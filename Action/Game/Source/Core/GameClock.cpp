#include "stdafx.h"
#include "GameClock.h"

float GameClock::s_now = 0.0f;

void GameClock::Advance(float deltaTime)
{
	s_now += deltaTime;
}

float GameClock::Now()
{
	return s_now;
}
