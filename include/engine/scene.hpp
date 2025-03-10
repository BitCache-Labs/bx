#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/hash_map.hpp>
#include <engine/memory.hpp>
#include <engine/guard.hpp>
#include <engine/ecs.hpp>
#include <engine/script.hpp>
#include <engine/gameobject.hpp>

class SceneManager;

struct BX_API SceneCallHandles
{
    ScriptHandle sceneNewFn{ SCRIPT_INVALID_HANDLE };
    ScriptHandle sceneUpdateFn{ SCRIPT_INVALID_HANDLE };

    ScriptHandle gameObjNewFn{ SCRIPT_INVALID_HANDLE };
    ScriptHandle gameObjStartFn{ SCRIPT_INVALID_HANDLE };
    ScriptHandle gameObjUpdateFn{ SCRIPT_INVALID_HANDLE };
};

class BX_API Scene
{
    BX_TYPE(Scene)

public:
    Scene(SceneManager& sceneMgr);
    ~Scene();

    inline SceneManager& GetSceneManager() { return m_sceneMgr; }

    inline void SetInstance(ScriptHandle instance) { m_instance = instance; }
    inline ScriptHandle GetInstance() const { return m_instance; }

    inline const List<SharedPtr<GameObject>>& GetGameObjects() const { return m_gameObjects; }

    void Update();

    GameObject& AddGameObject(ScriptHandle classHandle);
    void RemoveGameObject(GameObject* gameObj);

private:
    SceneManager& m_sceneMgr;
    ScriptHandle m_instance{ SCRIPT_INVALID_HANDLE };

    List<SharedPtr<GameObject>> m_pendingAdded{};
    List<SharedPtr<GameObject>> m_pendingRemoved{};
    List<SharedPtr<GameObject>> m_gameObjects{};
};

class BX_API SceneManager
{
    BX_TYPE(SceneManager)

public:
    SceneManager();
    virtual ~SceneManager();

public:
    inline EntityManager& GetEntityManager() { return m_entityMgr; }
    inline SystemManager& GetSystemManager() { return m_systemMgr; }
    inline GameObjectManager& GetGameObjectManager() { return m_gameObjMgr; }
    
    inline ScriptHandle GetVm() const { return m_vm; }
    inline const SceneCallHandles& GetCallHandles() const { return m_callHandles; }

    Scene& AddScene(StringView moduleName, StringView className, StringView filepath);
    Scene& AddScene();
    void RemoveScene(const SizeType index);

    inline const List<Scene*>& GetScenes() const { return m_scenes; }
    inline Scene* GetCurrentScene() { return m_currentScene; }

    void Update();

private:
    EntityManager m_entityMgr{};
    SystemManager m_systemMgr{};
    GameObjectManager m_gameObjMgr{};

    ScriptHandle m_vm{ SCRIPT_INVALID_HANDLE };
    SceneCallHandles m_callHandles{};

    //List<SharedPtr<Scene>> m_pendingAdded{};
    //List<SharedPtr<Scene>> m_pendingRemoved{};
    List<Scene*> m_scenes{};

    Scene* m_currentScene{ nullptr };
};