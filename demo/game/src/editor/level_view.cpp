#include <editor/level_view.hpp>

#include <bx/engine/imgui.hpp>

#include <rttr/registration.h>
RTTR_REGISTRATION
{
	rttr::registration::class_<LevelView>("LevelView")
	.constructor()
	.property("m_levelView", &LevelView::m_levelView);;
}

bool LevelView::Initialize()
{
	return true;
}

void LevelView::Reload()
{
}

void LevelView::Shutdown()
{
}

const char* LevelView::GetTitle() const
{
	return "Level";
}

void LevelView::Present(const char* title, bool& isOpen)
{
	ImGui::Begin(title, &isOpen);
	ImGui::End();
}