#pragma once

#include <resources/level.hpp>

#include <bx/bx.hpp>
#include <bx/editor/views/assets_view.hpp>

class GAME_API LevelView final : public AssetEditor
{
	RTTR_ENABLE(AssetEditor)

public:
	bool Initialize() override;
	void Reload() override;
	void Shutdown() override;

	const char* GetTitle() const override;
	void Present(const char* title, bool& isOpen) override;

private:
	RTTR_REGISTRATION_FRIEND
	bool m_levelView = true;
};