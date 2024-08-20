#include "bx/framework/gameobject.hpp"

#include <bx/engine/core/macros.hpp>
#include <bx/engine/core/file.hpp>
#include <bx/engine/core/application.hpp>
#include <bx/engine/containers/hash_map.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/polymorphic.hpp>

#include "bx/framework/systems/acoustics.hpp"
#include "bx/framework/systems/dynamics.hpp"
#include "bx/framework/systems/renderer.hpp"

#include "bx/framework/gameobject.serial.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>

static List<String> g_gameObjectClasses;
static HashMap<String, GameObjectMetaData> g_gameObjectMetaDataMap;

static Scene g_currentScene;

class GameObjectReceiver : public Receiver
{
public:
	void Receive(const EntityDestroyed& ev)
	{
		auto& gameObjects = g_currentScene.m_gameObjects;
		auto it = std::find_if(gameObjects.begin(), gameObjects.end(),
			[ev](const GameObjectBase* obj)
			{
				return obj->GetEntity() == ev.entity;
			});

		if (it != gameObjects.end())
			gameObjects.erase(it);
	}
};

static GameObjectReceiver g_receiver;

GameObjectData GameObjectData::Load(const String& filepath)
{
	GameObjectData gameObjData;

	std::ifstream stream(File::GetPath(filepath));
	cereal::JSONInputArchive ar(stream);
	ar(cereal::make_nvp("gameobject", gameObjData));

	return gameObjData;
}

void GameObjectData::Save(const String& filepath, const GameObjectData& data)
{
	std::ofstream stream(File::GetPath(filepath));
	cereal::JSONOutputArchive ar(stream);
	ar(cereal::make_nvp("gameobject", data));
}

GameObjectBase::GameObjectBase(Scene& scene, const GameObjectData& data, GameObjectBindObjectFn bindObjFn, GameObjectStartFn startFn, GameObjectUpdateFn updateFn)
	: m_scene(scene)
	, m_name(data.name)
	, m_className(data.className)
	, m_bindObjFn(bindObjFn)
	, m_startFn(startFn)
	, m_updateFn(updateFn)
{
	if (data.entity.GetId() == INVALID_ENTITY_ID)
		m_entity = EntityManager::CreateEntity();
	else
		m_entity = EntityManager::CreateEntityWithId(data.entity.GetId());

	m_scene.Add(this);
}

GameObjectBase::~GameObjectBase()
{
	if (m_entity.IsValid())
		m_entity.Destroy();
}

void GameObjectBase::Initialize(const GameObjectData& data)
{
	for (const auto& cmp : data.components)
	{
		for (auto cmpPtr : m_entity.GetComponents())
		{
			if (cmpPtr->GetTypeId() == cmp->GetTypeId())
			{
				cmpPtr->Copy(*cmp);
			}
		}
	}
}

void GameObjectBase::Destroy()
{
	m_scene.Remove(this);
}

bool GameObject::Initialize()
{
	EntityManager::Initialize();

	SystemManager::AddSystem<Dynamics>();
	SystemManager::AddSystem<Acoustics>();
	SystemManager::AddSystem<Renderer>();
	SystemManager::Initialize();

	Event::Subscribe<EntityDestroyed, GameObjectReceiver>(g_receiver);

	return true;
}

void GameObject::Reload()
{
	Shutdown();
	Initialize();
}

void GameObject::Shutdown()
{
	g_gameObjectMetaDataMap.clear();
	g_gameObjectClasses.clear();
	g_currentScene.m_gameObjects.clear();
	
	SystemManager::Shutdown();
	EntityManager::Shutdown();
}

void GameObject::Register(const String& className, const GameObjectMetaData& metaData)
{
	g_gameObjectClasses.emplace_back(className);
	g_gameObjectMetaDataMap.insert(std::make_pair(className, metaData));

	BX_LOGD("Registered GameObject class: {}", className);
}

const List<String>& GameObject::GetClasses()
{
	return g_gameObjectClasses;
}

const GameObjectMetaData& GameObject::GetClassMetaData(const String& className)
{
	auto it = g_gameObjectMetaDataMap.find(className);
	BX_ASSERT(it != g_gameObjectMetaDataMap.end(), "GameObject class not registered!");
	return it->second;
}

GameObjectBase& GameObject::New(Scene& scene, const String& className)
{
	GameObjectData data{ className, className, Entity::Invalid() };
	return NewFromData(scene, data);
}

GameObjectBase& GameObject::NewFromData(Scene& scene, const GameObjectData& data)
{
	const auto& metaData = GetClassMetaData(data.className);
	metaData.bindClassFn();
	if (!metaData.constructFn(data))
	{
		throw std::runtime_error("Failed to construct game object!");
	}

	return *scene.m_gameObjects[scene.m_gameObjects.size() - 1];
}

GameObjectBase& GameObject::Load(Scene& scene, const String& filepath)
{
	GameObjectData gameObjData = GameObjectData::Load(filepath);

	auto& obj = GameObject::NewFromData(Scene::GetCurrent(), gameObjData);
	obj.SetName(gameObjData.name);

	SizeType i = 0;
	for (auto cmpPtr : obj.GetEntity().GetComponents())
	{
		cmpPtr->Copy(*gameObjData.components[i++]);
	}

	return obj;
}

void GameObject::Save(const GameObjectBase& gameObj, const String& filepath)
{
	GameObjectData gameObjData;
	gameObjData.name = gameObj.GetName();
	gameObjData.className = gameObj.GetClassName();
	gameObjData.entity = Entity::Invalid();

	const auto& cmpPtrs = gameObj.GetEntity().GetComponents();
	gameObjData.components.reserve(cmpPtrs.size());
	for (const auto cmpPtr : cmpPtrs)
	{
		gameObjData.components.emplace_back(cmpPtr, [](ComponentBase*) {});
	}

	GameObjectData::Save(filepath, gameObjData);
}

GameObjectBase& GameObject::Find(Scene& scene, EntityId entityId)
{
	for (auto& gameObj : scene.m_gameObjects)
		if (gameObj->GetEntity().GetId() == entityId)
			return *gameObj;
	BX_FAIL("No game object found for entity ID!");
}

GameObjectBase& GameObject::Duplicate(const GameObjectBase& gameObj)
{
	String name = gameObj.GetName();
	Entity entity = gameObj.GetEntity();

	auto& copy = GameObject::New(Scene::GetCurrent(), gameObj.GetClassName());
	copy.SetName(name);

	const auto& cmps = entity.GetComponents();
	const auto& copyCmps = copy.GetEntity().GetComponents();
	for (SizeType i = 0; i < cmps.size(); ++i)
	{
		copyCmps[i]->Copy(*cmps[i]);
	}
	return copy;
}

void Scene::Create(const String& filename)
{
	Scene scene;
	Save(scene, filename);
}

void Scene::Load(const String& filename)
{
	Load(g_currentScene, filename);
}

void Scene::Save(const String& filename)
{
	Save(g_currentScene, filename);
}

void Scene::Load(Scene& scene, const String& filename)
{
	if (!File::Exists(filename))
	{
		BX_LOGW("Failed to load non-existent scene: {}", filename);
		return;
	}

	scene.m_gameObjects.clear();
	scene.m_pendingAdded.clear();
	scene.m_pendingRemoved.clear();

	try
	{
		std::ifstream stream(File::GetPath(filename));
		cereal::JSONInputArchive ar(stream);
		ar(cereal::make_nvp("scene", scene));
	}
	catch (cereal::Exception& e)
	{
		BX_LOGW("Failed to load scene ({}): {}", filename, e.what());
	}
}

void Scene::Save(const Scene& scene, const String& filename)
{
	try
	{
		std::ofstream stream(File::GetPath(filename));
		cereal::JSONOutputArchive ar(stream);
		ar(cereal::make_nvp("scene", scene));
	}
	catch (cereal::Exception& e)
	{
		BX_LOGW("Failed to save scene ({}): {}", filename, e.what());
	}
}

Scene& Scene::GetCurrent()
{
	return g_currentScene;
}

void Scene::Update()
{
	// Ensure all gameobjects are started before update
	List<GameObjectBase*> added = m_pendingAdded;
	while (!added.empty())
	{
		m_pendingAdded.clear();

		// Start game objects
		for (auto& go : added)
			go->Start();

		added = m_pendingAdded;
	}

	// Update game objects
	for (const auto& go : m_gameObjects)
		go->Update();

	// Remove pending objects
	for (const auto& go : m_pendingRemoved)
		go->GetEntity().Destroy();
	m_pendingRemoved.clear();
}