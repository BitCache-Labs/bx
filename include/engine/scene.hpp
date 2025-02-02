#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/hash_map.hpp>
#include <engine/memory.hpp>
#include <engine/guard.hpp>

using SceneHandle = u64;
constexpr SceneHandle SCENE_INVALID_HANDLE = -1;

class Scene;

class BX_API SceneManager
{
    BX_TYPE(SceneManager)

public:
    virtual ~SceneManager() = default;
    virtual bool CanAddScene() = 0;

public:
    template <typename T, typename... Args>
    SceneHandle CreateScene(Args&&... args);
    inline void DestroyScene(SceneHandle scene);
    inline Scene* GetScene(SceneHandle scene) const;

    inline void SetActiveScene(SceneHandle scene);
    inline SceneHandle GetActiveScene() const;

    inline const HashMap<SceneHandle, UniquePtr<Scene>>& GetScenes() const;

private:
    SceneHandle m_activeScene{ SCENE_INVALID_HANDLE };
    HashMap<SceneHandle, UniquePtr<Scene>> m_scenes{};
};

class BX_API Scene
{
    BX_TYPE(Scene)

public:
    virtual ~Scene() = default;

    virtual void OnCreate() = 0;
    virtual void OnDestroy() = 0;

    virtual void OnActiveSceneChanged(SceneHandle oldScene, SceneHandle newScene) = 0;

    virtual void OnPlay() = 0;
    virtual void OnPause() = 0;
    virtual void OnStop() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;

public:
    inline SceneManager& GetManager() { return *m_pManager; }
    
    inline void SetHeadless(bool isHeadless) { m_isHeadless = isHeadless; }
    inline bool IsHeadless() const { return m_isHeadless; }

private:
    friend class SceneManager;
    SceneManager* m_pManager{ nullptr };
    bool m_isHeadless{ false };
};

template <typename T, typename... Args>
SceneHandle SceneManager::CreateScene(Args&&... args)
{
    static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");

    static SceneHandle g_counter = 0;
    SceneHandle handle = SCENE_INVALID_HANDLE;
    if (CanAddScene())
    {
        handle = g_counter++;
        auto scene = meta::make_unique<T>(std::forward<Args>(args)...);
        scene->m_pManager = this;
        scene->OnCreate();
        m_scenes.insert(std::make_pair(handle, std::move(scene)));
    }
    return handle;
}

inline void SceneManager::DestroyScene(SceneHandle scene)
{
    auto it = m_scenes.find(scene);
    BX_ENSURE(it != m_scenes.end());
    
    it->second->OnStop();
    it->second->OnDestroy();
    
    m_scenes.erase(it);

    if (m_activeScene == scene)
        m_activeScene = SCENE_INVALID_HANDLE;
}

inline Scene* SceneManager::GetScene(SceneHandle scene) const
{
    if (scene == SCENE_INVALID_HANDLE)
        return nullptr;

    auto it = m_scenes.find(scene);
    BX_ENSURE(it != m_scenes.end());
    return it->second.get();
}

inline void SceneManager::SetActiveScene(SceneHandle scene)
{
    if (m_activeScene == scene)
        return;

    if (m_activeScene != SCENE_INVALID_HANDLE)
    {
        auto it = m_scenes.find(m_activeScene);
        BX_ENSURE(it != m_scenes.end());
        it->second->OnStop();
    }

    for (const auto& it : m_scenes)
        it.second->OnActiveSceneChanged(m_activeScene, scene);

    m_activeScene = scene;

    auto it = m_scenes.find(m_activeScene);
    BX_ENSURE(it != m_scenes.end());
    it->second->OnPlay();
}

inline SceneHandle SceneManager::GetActiveScene() const { return m_activeScene; }

inline const HashMap<SceneHandle, UniquePtr<Scene>>& SceneManager::GetScenes() const { return m_scenes; }