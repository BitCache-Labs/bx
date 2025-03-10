#include <engine/gameobject.hpp>
#include <engine/scene.hpp>

// TODO: Remove this
#include <engine/wren/script_wren.hpp>

static const StringView g_gameObjSrc = R"(
//import "ecs" for Entity

foreign class GameObjectBase {

    foreign classHandle

    foreign instance
    foreign instance=(v)
    
    foreign name
    foreign name=(v)
    
    //foreign entity

    foreign destroy()

    foreign static register(name, type)
    foreign static create(type)
    foreign static load(filename)
}

class GameObject {
    construct new(base) {
        _base = base
        _base.instance = this
    }
    
    // Bindings
    name { _base.name }
    name=(v) { _base.name = v }
    
    //isValid { _base.entity.isValid }
    //hasComponent(cmp) { _base.entity.hasComponent(cmp) }
    //addComponent(cmp) { _base.entity.addComponent(cmp) }
    //getComponent(cmp) { _base.entity.getComponent(cmp) }
    //removeComponent(cmp) { _base.entity.removeComponent(cmp) }
    
    // Entry points for game logic
    start() {}
    update() {}

    destroy() { _base.destroy() }

    // Public static functions
    static register(type) { GameObjectBase.register(type.name, type) }

    static create(type) {
        var base = GameObjectBase.create(type)
        var inst = type.new(base)
        return inst
    }

    static load(filename) {
        var base = GameObjectBase.load(filename)
        var inst = base.classHandle.new(base)
        return inst
    }
}
)";

BX_SCRIPT_API_REGISTRATION(GameObject)
{
    auto& script = Script::Get();
    script.BeginModule("gameobject");
    {
        script.BeginClass<GameObject>("GameObjectBase");
        {
            script.BindFunction(false, "classHandle",
                [](ScriptHandle vm)
                {
                    auto& gameObj = Script::Get().GetSlotArg<GameObject&>(vm, 0);
                    Script::Get().SetSlotHandle(vm, 0, gameObj.GetClassHandle());
                });

            script.BindFunction(false, "instance",
                [](ScriptHandle vm)
                {
                    auto& gameObj = Script::Get().GetSlotArg<GameObject&>(vm, 0);
                    Script::Get().SetSlotHandle(vm, 0, gameObj.GetInstance());
                });

            script.BindFunction(false, "instance=(_)",
                [](ScriptHandle vm)
                {
                    auto& gameObj = Script::Get().GetSlotArg<GameObject&>(vm, 0);
                    auto instance = Script::Get().GetSlotHandle(vm, 1);
                    gameObj.SetInstance(instance);
                });

            script.BindFunction<decltype(&GameObject::GetName), &GameObject::GetName>(false, "name");
            script.BindFunction<decltype(&GameObject::SetName), &GameObject::SetName>(false, "name=(_)");

            //script.BindFunction<decltype(GameObject::GetEntity), &GameObject::GetEntity>(false, "entity");

            script.BindFunction<decltype(&GameObject::Destroy), &GameObject::Destroy>(false, "destroy()");

            script.BindFunction(true, "register(_,_)",
                [](ScriptHandle vm)
                {
                    auto& script = Script::Get();
                    script.EnsureSlots(vm, 1);

                    const auto className = script.GetSlotArg<StringView>(vm, 1);
                    const auto classHandle = script.GetSlotHandle(vm, 2);

                    GameObjectClass gameObjClass{ script.GetCurrentModule(), className, classHandle };
                    
                    auto sceneMgr = static_cast<SceneManager*>(script.GetUserData(vm));
                    sceneMgr->GetGameObjectManager().RegisterGameObject(gameObjClass);
                });

            script.BindFunction(true, "create(_)",
                [](ScriptHandle vm)
                {
                    auto& script = Script::Get();

                    ScriptHandle classHandle = script.GetSlotHandle(vm, 1);

                    auto sceneMgr = static_cast<SceneManager*>(script.GetUserData(vm));
                    auto scene = sceneMgr->GetCurrentScene();
                    auto& gameObjMgr = sceneMgr->GetGameObjectManager();
                    auto& gameObj = gameObjMgr.CreateGameObject(scene, classHandle);
                    
                    script.SetSlotArg(vm, 0, gameObj);
                });

            script.BindFunction(true, "load(_)",
                [](ScriptHandle vm)
                {
                    auto& script = Script::Get();

                    StringView filepath = script.GetSlotArg<StringView>(vm, 1);

                    auto sceneMgr = static_cast<SceneManager*>(script.GetUserData(vm));
                    auto scene = sceneMgr->GetCurrentScene();
                    auto& gameObjMgr = sceneMgr->GetGameObjectManager();
                    auto& gameObj = gameObjMgr.LoadGameObject(sceneMgr->GetCurrentScene(), filepath);

                    script.SetSlotArg(vm, 0, gameObj);
                });
        }
        script.EndClass();
    }
    script.EndModule();

    ScriptModuleSource src{};
    src.moduleName = "gameobject";
    src.moduleSource = g_gameObjSrc;
    return src;
}

GameObject::GameObject(Scene& scene, ScriptHandle classHandle)
    : m_scene(scene)
    , m_classHandle(classHandle)
{
}

GameObject::~GameObject()
{
    BX_ENSURE(m_classHandle != SCRIPT_INVALID_HANDLE);
    BX_ENSURE(m_instance != SCRIPT_INVALID_HANDLE);

    auto& script = Script::Get();
    const auto vm = m_scene.GetSceneManager().GetVm();
    script.ReleaseHandle(vm, m_classHandle);
    script.ReleaseHandle(vm, m_instance);
}

void GameObject::Start()
{
    if (m_instance == SCRIPT_INVALID_HANDLE)
        return;

    auto& script = Script::Get();
    const auto vm = m_scene.GetSceneManager().GetVm();
    const auto& handles = m_scene.GetSceneManager().GetCallHandles();

    script.SetSlotHandle(vm, 0, m_instance);
    script.CallFunction(vm, handles.gameObjStartFn);
}

void GameObject::Update()
{
    if (m_instance == SCRIPT_INVALID_HANDLE)
        return;

    auto& script = Script::Get();
    const auto vm = m_scene.GetSceneManager().GetVm();
    const auto& handles = m_scene.GetSceneManager().GetCallHandles();

    script.SetSlotHandle(vm, 0, m_instance);
    script.CallFunction(vm, handles.gameObjUpdateFn);
}

void GameObject::Destroy()
{
    m_scene.RemoveGameObject(this);
}

GameObjectManager::GameObjectManager()
{
}

GameObjectManager::~GameObjectManager()
{
}

void GameObjectManager::RegisterGameObject(const GameObjectClass& gameObjClass)
{
    m_classes.emplace_back(gameObjClass);
}

GameObject& GameObjectManager::CreateGameObject(Scene* scene, ScriptHandle classHandle)
{
    BX_ENSURE(scene != nullptr);

    const auto& t = GetClass(classHandle);

    return scene->AddGameObject(classHandle);
}

GameObject& GameObjectManager::CreateGameObject(Scene* scene, const GameObjectClass& gameObjClass)
{
    BX_ENSURE(scene != nullptr);

    auto& script = Script::Get();
    const auto vm = scene->GetSceneManager().GetVm();

    auto classHandle = script.MakeClassHandle(vm, gameObjClass.classModule, gameObjClass.className);
    auto& gameObj = scene->AddGameObject(classHandle);
    
    script.EnsureSlots(vm, 2);
    script.SetSlotHandle(vm, 0, classHandle);
    script.SetSlotArg(vm, 1, gameObj);
    
    const auto& handles = scene->GetSceneManager().GetCallHandles();
    script.CallFunction(vm, handles.gameObjNewFn);

    auto instance = script.GetSlotHandle(vm, 0);
    gameObj.SetInstance(instance);

    return gameObj;
}

GameObject& GameObjectManager::LoadGameObject(Scene* scene, const StringView filepath)
{
    // TODO: For testing, remove later with loaded stuff
    const auto& gameObjClass = m_classes.front();

    return CreateGameObject(scene, gameObjClass);
}

const GameObjectClass& GameObjectManager::GetClass(ScriptHandle handle) const
{
    auto& script = Script::Get();
    u64 value = script.GetHandleValue(handle);

    for (const auto& gameObjClass : m_classes)
    {
        if (value == script.GetHandleValue(gameObjClass.classHandle))
            return gameObjClass;
    }

    BX_FAIL("Class does not exist!");
    throw;
}