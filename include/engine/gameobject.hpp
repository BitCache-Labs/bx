#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/ecs.hpp>
#include <engine/string.hpp>
#include <engine/script.hpp>

class Scene;
class SceneManager;

struct BX_API GameObjectClass
{
	CString<64> classModule{};
	CString<64> className{};
	ScriptHandle classHandle{ SCRIPT_INVALID_HANDLE };
};

class BX_API GameObject
{
	BX_TYPE(GameObject)

public:
	GameObject(Scene& scene, ScriptHandle classHandle);
	~GameObject();

	inline Scene& GetScene() { return m_scene; }
	inline ScriptHandle GetClassHandle() const { return m_classHandle; }

	inline void SetInstance(ScriptHandle instance) { m_instance = instance; }
	inline ScriptHandle GetInstance() const { return m_instance; }

	inline StringView GetName() const { return m_name; }
	inline void SetName(StringView name) { m_name = name; }

	inline Entity GetEntity() const { return m_entity; }

	void Start();
	void Update();

	void Destroy();

private:
	Scene& m_scene;
	ScriptHandle m_classHandle{ SCRIPT_INVALID_HANDLE };

	ScriptHandle m_instance{ SCRIPT_INVALID_HANDLE };

	Entity m_entity{};
	CString<64> m_name{};
};

class BX_API GameObjectManager
{
public:
	GameObjectManager();
	~GameObjectManager();

	void RegisterGameObject(const GameObjectClass& gameObjClass);
	GameObject& CreateGameObject(Scene* scene, ScriptHandle classHandle);
	GameObject& CreateGameObject(Scene* scene, const GameObjectClass& gameObjClass);
	GameObject& LoadGameObject(Scene* scene, const StringView filepath);

	inline const List<GameObjectClass>& GetClasses() const { return m_classes; }
	const GameObjectClass& GetClass(ScriptHandle handle) const;

private:
	List<GameObjectClass> m_classes{};
};