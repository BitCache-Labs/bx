#pragma once

#include <engine/api.hpp>
#include <engine/scene.hpp>

class BX_API World final
	: public SceneManager
{
	BX_TYPE(World, SceneManager)

public:
	World();
	~World();

	void OnInitialize();
	void OnShutdown();

	void OnUpdate();
	void OnRender();

private:
	friend class WorldEditor;

	SceneHandle m_scene{ SCENE_INVALID_HANDLE };

	// TODO: Move these to WorldEditor
	bool m_playing{ false };
	bool m_paused{ false };
};