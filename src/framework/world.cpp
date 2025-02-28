#include <framework/world.hpp>

#include <engine/math.hpp>
#include <engine/ecs.hpp>
#include <engine/script.hpp>
#include <engine/string.hpp>
#include <engine/list.hpp>
#include <engine/hash_map.hpp>
#include <engine/function.hpp>
#include <engine/memory.hpp>
#include <engine/macros.hpp>
#include <engine/file.hpp>
#include <engine/application.hpp>
#include <engine/hash_map.hpp>

#include <engine/wren/script_wren.hpp>

World::World()
{
}

World::~World()
{
}

void World::OnInitialize()
{
	m_scene = CreateScene();
}

void World::OnShutdown()
{
	DestroyScene(m_scene);
}

void World::OnUpdate()
{
}

void World::OnRender()
{
}