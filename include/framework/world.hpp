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
	
private:
	friend class WorldEditor;
};