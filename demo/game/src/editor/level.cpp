#include <editor/level.hpp>

#include <bx/engine/imgui.hpp>

#include <rttr/registration.h>
RTTR_REGISTRATION
{
	rttr::registration::class_<Level>("Level").constructor();
}

Level::Level()
{
}

bool Level::Initialize()
{
	return true;
}

void Level::Reload()
{
}

void Level::Shutdown()
{
}

void Level::Present()
{
	ImGui::Begin("Level", &m_open);
	ImGui::End();
}