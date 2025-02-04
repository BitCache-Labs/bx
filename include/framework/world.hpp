#pragma once

#include <engine/api.hpp>
#include <engine/scene.hpp>
#include <engine/math.hpp>
//#include <engine/ecs.hpp>
#include <engine/script.hpp>
#include <engine/string.hpp>
#include <engine/list.hpp>
#include <engine/hash_map.hpp>
#include <engine/function.hpp>

class BX_API World final
	: public Scene
{
	BX_TYPE(World, Scene)

public:
	void OnCreate() override;
	void OnDestroy() override;

	void OnActiveSceneChanged(SceneHandle oldScene, SceneHandle newScene) override;

	void OnPlay() override;
	void OnPause() override;
	void OnStop() override;
	void OnUpdate() override;
	void OnRender() override;

public:
	void Reload();

private:
	friend class WorldEditor;

	bool m_playing{ false };
	bool m_paused{ false };

	ScriptHandle m_vm{ SCRIPT_INVALID_HANDLE };
	ScriptHandle m_newFn{ SCRIPT_INVALID_HANDLE };
	ScriptHandle m_updateFn{ SCRIPT_INVALID_HANDLE };
	ScriptHandle m_renderFn{ SCRIPT_INVALID_HANDLE };
	ScriptHandle m_instance{ SCRIPT_INVALID_HANDLE };
};

/*
struct GameObjectData;
class GameObjectBase;
class GameObject;
class Scene;

using GameObjectBindClassFn = Function<void()>;
using GameObjectConstructFn = Function<bool(const GameObjectData&)>;

using GameObjectBindObjectFn = Function<void()>;
using GameObjectStartFn = Function<void()>;
using GameObjectUpdateFn = Function<void()>;

struct GameObjectMetaData
{
	GameObjectBindClassFn bindClassFn;
	GameObjectConstructFn constructFn;
};

struct GameObjectData
{
	String name;
	String className;
	Entity entity;
	List<std::shared_ptr<ComponentBase>> components;

	static GameObjectData Load(const String& filepath);
	static void Save(const String& filepath, const GameObjectData& data);
};

class GameObjectBase
{
public:
	GameObjectBase(Scene& scene, const GameObjectData& data, GameObjectBindObjectFn bindObjFn, GameObjectStartFn startFn, GameObjectUpdateFn updateFn);
	~GameObjectBase();

	void Initialize(const GameObjectData& data);
	void Destroy();

	inline const String& GetClassName() const { return m_className; }

	inline const String& GetName() const { return m_name; }
	inline void SetName(const String& name) { m_name = name; }

	inline Entity GetEntity() const { return m_entity; }

	inline void Bind() const
	{
		ENGINE_ASSERT(m_bindObjFn, "No bind function bound!");
		m_bindObjFn();
	}

	inline void Start()
	{
		ENGINE_ENSURE(m_started == false);
		m_started = true;

		ENGINE_ASSERT(m_startFn, "No start function bound!");
		try
		{
			m_startFn();
		}
		catch (std::exception& e)
		{
		}
	}

	inline void Update() const
	{
		ENGINE_ASSERT(m_updateFn, "No update function bound!");
		m_updateFn();
	}

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Inspector;

	bool m_started = false;

	Scene& m_scene;
	Entity m_entity;

	String m_name;
	String m_className;
	GameObjectBindObjectFn m_bindObjFn;
	GameObjectStartFn m_startFn;
	GameObjectUpdateFn m_updateFn;
};

// Wrapper around script object
class GameObject
{
public:
	// TODO: Split these static function into a GameObjectManager?
	static void Initialize();
	static void Shutdown();

	static void Register(const String& className, const GameObjectMetaData& metaData);
	static const List<String>& GetClasses();
	static const GameObjectMetaData& GetClassMetaData(const String& className);

	static GameObjectBase& New(Scene& scene, const String& className);
	static GameObjectBase& NewFromData(Scene& scene, const GameObjectData& data);

	static GameObjectBase& Load(Scene& scene, const String& filepath);
	static void Save(const GameObjectBase& gameObj, const String& filepath);

	static GameObjectBase& Find(Scene& scene, EntityId entityId);
	static GameObjectBase& Duplicate(const GameObjectBase& gameObj);
};

class Scene
{
public:
	// TODO: Split these static function into a SceneManager?
	static void Create(const String& filename);
	static void Load(const String& filename);
	static void Save(const String& filename);

	static Scene& GetCurrent();

	void Update();

private:
	static void Load(Scene& scene, const String& filename);
	static void Save(const Scene& scene, const String& filename);

public:
	inline const List<GameObjectBase*>& GetGameObjects() const { return m_gameObjects; }

private:
	inline void Add(GameObjectBase* gameObj)
	{
		m_pendingAdded.emplace_back(gameObj);
		m_gameObjects.emplace_back(gameObj);
	}

	inline void Remove(GameObjectBase* gameObj)
	{
		m_pendingRemoved.emplace_back(gameObj);
		auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
			[gameObj](const GameObjectBase* i)
			{
				return i->GetEntity() == gameObj->GetEntity();
			});
		m_gameObjects.erase(it);
	}

private:
	friend class GameObject;
	friend class GameObjectBase;
	friend class GameObjectReceiver;

	template <typename T>
	friend class Serial;

	List<GameObjectBase*> m_pendingAdded;
	List<GameObjectBase*> m_pendingRemoved;
	List<GameObjectBase*> m_gameObjects;
};
*/