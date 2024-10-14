#include <game.hpp>

#include <rttr/registration.h>
RTTR_REGISTRATION
{
	rttr::registration::class_<Game>("Game").constructor();
}

Game::Game()
{
}

bool Game::Initialize()
{
	return true;
}

void Game::Reload()
{
}

void Game::Shutdown()
{
}