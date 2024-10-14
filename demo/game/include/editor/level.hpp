#pragma once

#include <bx/bx.hpp>
#include <bx/editor/views/assets_view.hpp>

class GAME_API Level final : public AssetEditor
{
	RTTR_ENABLE(AssetEditor)

public:
	Level();

	bool Initialize() override;
	void Reload() override;
	void Shutdown() override;

	void Present() override;

private:
};