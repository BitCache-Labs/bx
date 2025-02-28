#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/hash_map.hpp>
#include <engine/memory.hpp>
#include <engine/guard.hpp>
#include <engine/ecs.hpp>
#include <engine/script.hpp>
#include <engine/gameobject.hpp>

using SceneHandle = u64;
constexpr SceneHandle SCENE_INVALID_HANDLE = -1;

class Scene;

class BX_API SceneManager
{
    BX_TYPE(SceneManager)

public:
    SceneManager();
    virtual ~SceneManager();

public:
    SceneHandle CreateScene();
    void DestroyScene(SceneHandle scene);

    Scene& GetScene(SceneHandle scene);
    inline const HashMap<SceneHandle, Scene>& GetScenes() const { return m_scenes; }

    void ClearScenes();

private:
    friend class Scene;

    HashMap<SceneHandle, Scene> m_scenes{};

    EntityManager m_entityMgr{};
    SystemManager m_systemMgr{};

    ScriptHandle m_vm{ SCRIPT_INVALID_HANDLE };
};

class BX_API Scene
{
    BX_TYPE(Scene)

public:
    Scene() = default;
    ~Scene() = default;

    inline SceneManager& GetManager() { return *m_pManager; }
    
    inline const List<SharedPtr<GameObject>>& GetGameObjects() const { return m_gameObjects; }

    void OnCreate();
    void OnDestroy();

    void OnUpdate();

private:
    friend class SceneManager;

    inline void Add(const SharedPtr<GameObject>& gameObj)
    {
        m_pendingAdded.emplace_back(gameObj);
        m_gameObjects.emplace_back(gameObj);
    }

    inline void Remove(const SharedPtr<GameObject>& gameObj)
    {
        m_pendingRemoved.emplace_back(gameObj);
        auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
            [gameObj](const SharedPtr<GameObject>& i)
            {
                return i->GetEntity() == gameObj->GetEntity();
            });
        m_gameObjects.erase(it);
    }

private:
    SceneManager* m_pManager{ nullptr };

    ScriptHandle m_newFn{ SCRIPT_INVALID_HANDLE };
    ScriptHandle m_updateFn{ SCRIPT_INVALID_HANDLE };
    ScriptHandle m_instance{ SCRIPT_INVALID_HANDLE };

    List<SharedPtr<GameObject>> m_pendingAdded{};
    List<SharedPtr<GameObject>> m_pendingRemoved{};
    List<SharedPtr<GameObject>> m_gameObjects{};
};