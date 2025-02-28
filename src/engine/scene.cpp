#include <engine/scene.hpp>

#include <engine/file.hpp>

// TODO: Remove this
#include <engine/wren/script_wren.hpp>

SceneManager::SceneManager()
{
	ScriptVmInfo info{};
	m_vm = Script::Get().CreateVm(info);
}

SceneManager::~SceneManager()
{
	Script::Get().DestroyVm(m_vm);
}

SceneHandle SceneManager::CreateScene()
{
	static SceneHandle g_counter = 0;
	SceneHandle handle = g_counter++;
	
	Scene scene{};
	scene.m_pManager = this;
	scene.OnCreate();

	m_scenes.insert(std::make_pair(handle, std::move(scene)));

	return handle;
}

void SceneManager::DestroyScene(SceneHandle scene)
{
	auto it = m_scenes.find(scene);
	BX_ENSURE(it != m_scenes.end());

	it->second.OnDestroy();
	
	m_scenes.erase(it);
}

Scene& SceneManager::GetScene(SceneHandle scene)
{
	BX_ENSURE(scene != SCENE_INVALID_HANDLE);
	auto it = m_scenes.find(scene);
	BX_ENSURE(it != m_scenes.end());
	return it->second;
}

void SceneManager::ClearScenes()
{
	m_scenes.clear();
}

void Scene::OnCreate()
{
	m_newFn = Script::Get().CreateFunction(m_pManager->m_vm, "new()");
	m_updateFn = Script::Get().CreateFunction(m_pManager->m_vm, "update()");

	Script::Get().CompileFile(m_pManager->m_vm, "test", File::Get().GetPath("[assets]/test.wren"));

	Script::Get().EnsureSlots(m_pManager->m_vm, 1);

	// TODO: Remove this, make into API
	wrenGetVariable((WrenVM*)m_pManager->m_vm, "test", "Test", 0);
	auto testClass = Script::Get().GetSlotHandle(m_pManager->m_vm, 0);
	Script::Get().SetSlotHandle(m_pManager->m_vm, 0, testClass);

	Script::Get().CallFunction(m_pManager->m_vm, m_newFn);
	m_instance = Script::Get().GetSlotHandle(m_pManager->m_vm, 0);

	auto types = GameObject::GetTypes(m_pManager->m_vm);
}

void Scene::OnDestroy()
{
	Script::Get().DestroyFunction(m_pManager->m_vm, m_newFn);
	Script::Get().DestroyFunction(m_pManager->m_vm, m_updateFn);
}

void Scene::OnUpdate()
{
	Script::Get().SetSlotHandle(m_pManager->m_vm, 0, m_instance);
	Script::Get().CallFunction(m_pManager->m_vm, m_updateFn);

	// Ensure all gameobjects are started before update
	List<SharedPtr<GameObject>> added = m_pendingAdded;
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