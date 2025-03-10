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

World::World()
{
}

World::~World()
{
}

void World::OnInitialize()
{
	AddScene("test", "Test", "[assets]/test.wren");
}

void World::OnShutdown()
{
	//DestroyScene(m_scene);
}