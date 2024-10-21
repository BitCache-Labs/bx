#include <game.hpp>

#ifdef BX_EDITOR_BUILD
#include <editor/level_view.hpp>
#endif

#include <rttr/registration.h>
RTTR_REGISTRATION
{
	rttr::registration::class_<Game>("Game").constructor();
}

Game::Game()
{
}

//#include <cereal/archives/json.hpp>
//#include <cereal/details/polymorphic_impl.hpp>

bool Game::Initialize()
{
#ifdef BX_EDITOR_BUILD
	ImGui::SetCurrentContext(ImGuiManager::GetCurrentContext());
	AssetsManager::Get().RegisterEditor(Type<Level>::Id(), []() -> AssetEditor* { return new LevelView(); });
#endif

	AssetsManager::Get().OpenEditor(0);

	return true;
}

void Game::Reload()
{
}

void Game::Shutdown()
{
}