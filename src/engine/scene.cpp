#include <engine/scene.hpp>
#include <engine/file.hpp>

Scene::Scene(SceneManager& sceneMgr)
	: m_sceneMgr(sceneMgr)
{
}

Scene::~Scene()
{
	m_gameObjects.clear();

	BX_ENSURE(m_instance != SCRIPT_INVALID_HANDLE);
	Script::Get().ReleaseHandle(m_sceneMgr.GetVm(), m_instance);
}

void Scene::Update()
{
	auto& script = Script::Get();
	const auto vm = m_sceneMgr.GetVm();
	const auto& handles = m_sceneMgr.GetCallHandles();

	script.SetSlotHandle(vm, 0, m_instance);
	script.CallFunction(vm, handles.sceneUpdateFn);

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
	for (auto& go : m_gameObjects)
		go->Update();

	// Remove pending objects
	for (auto& go : m_pendingRemoved)
		go->GetEntity().Destroy();
	
	m_pendingRemoved.clear();
}

GameObject& Scene::AddGameObject(ScriptHandle classHandle)
{
	SharedPtr<GameObject> gameObj(new GameObject(*this, classHandle));
	m_pendingAdded.emplace_back(gameObj);
	m_gameObjects.emplace_back(gameObj);
	return *gameObj;
}

void Scene::RemoveGameObject(GameObject* gameObj)
{
	auto it = m_gameObjects.end();
	for (auto i = m_gameObjects.begin(); i != m_gameObjects.end(); ++i)
	{
		if (i->get() == gameObj)
		{
			it = i;
			break;
		}
	}
	BX_ENSURE(it != m_gameObjects.end());

	m_pendingRemoved.emplace_back(*it);
	m_gameObjects.erase(it);
}

SceneManager::SceneManager()
{
	auto& script = Script::Get();

	ScriptVmInfo info{};
	m_vm = script.CreateVm(info);
	script.SetUserData(m_vm, this);

	m_callHandles.sceneNewFn = script.MakeCallHandle(m_vm, "new()");
	m_callHandles.sceneUpdateFn = script.MakeCallHandle(m_vm, "update()");

	m_callHandles.gameObjNewFn = script.MakeCallHandle(m_vm, "new(_)");
	m_callHandles.gameObjStartFn = script.MakeCallHandle(m_vm, "start()");
	m_callHandles.gameObjUpdateFn = script.MakeCallHandle(m_vm, "update()");
}

SceneManager::~SceneManager()
{
	for (auto scene : m_scenes)
		delete scene;

	auto& script = Script::Get();

	script.ReleaseHandle(m_vm, m_callHandles.sceneNewFn);
	script.ReleaseHandle(m_vm, m_callHandles.sceneUpdateFn);

	script.ReleaseHandle(m_vm, m_callHandles.gameObjNewFn);
	script.ReleaseHandle(m_vm, m_callHandles.gameObjStartFn);
	script.ReleaseHandle(m_vm, m_callHandles.gameObjUpdateFn);

	for (const auto& gameObjClass : m_gameObjMgr.GetClasses())
	{
		script.ReleaseHandle(m_vm, gameObjClass.classHandle);
	}

	script.DestroyVm(m_vm);
}

Scene& SceneManager::AddScene(StringView moduleName, StringView className, StringView filepath)
{
	auto scene = new Scene(*this);
	m_currentScene = scene;

	auto& script = Script::Get();
	script.CompileFile(m_vm, moduleName, File::Get().GetPath(filepath));
	
	script.EnsureSlots(m_vm, 1);
	
	auto sceneClass = script.MakeClassHandle(m_vm, moduleName, className);
	script.SetSlotHandle(m_vm, 0, sceneClass);

	script.CallFunction(m_vm, m_callHandles.sceneNewFn);
	auto instance = script.GetSlotHandle(m_vm, 0);

	script.ReleaseHandle(m_vm, sceneClass);

	scene->SetInstance(instance);
	m_scenes.emplace_back(scene);

	m_gameObjMgr.CreateGameObject(scene, m_gameObjMgr.GetClasses().front());

	m_currentScene = nullptr;
	return *scene;
}

Scene& SceneManager::AddScene()
{
	auto scene = new Scene(*this);
	m_scenes.emplace_back(scene);
	return *scene;
}

void SceneManager::RemoveScene(const SizeType index)
{
	BX_FAIL("TODO");
}

void SceneManager::Update()
{
	for (auto scene : m_scenes)
	{
		m_currentScene = scene;
		scene->Update();
	}
	m_currentScene = nullptr;
}