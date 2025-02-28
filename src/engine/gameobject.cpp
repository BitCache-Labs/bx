#include <engine/gameobject.hpp>

// TODO: Remove this
#include <engine/wren/script_wren.hpp>

static const StringView g_gameObjSrc = R"(
//import "ecs" for Entity

foreign class GameObjectType {
    construct new() {}
}

class GameObject {
    construct new() {
    }
    
    // Entry points for game logic
    start() {}
    update() {}

    // Public static functions
    static register(type) {
        types.add(GameObjectType.new())
    }
    
    static types {
        if (__types == null) {
            __types = []
        }
        return __types
    }
}
)";

BX_SCRIPT_API_REGISTRATION(GameObject)
{
    Script::Get().BeginModule("gameobject");
    {
        Script::Get().BeginClass<GameObjectType>("GameObjectType");
        {
            //Script::Get().BindFunction(true, "test()", [](ScriptHandle vm) { Audio::Get().Test(); });
        }
        Script::Get().EndClass();
    }
    Script::Get().EndModule();

    ScriptModuleSource src{};
    src.moduleName = "gameobject";
    src.moduleSource = g_gameObjSrc;
    return src;
}

List<GameObjectType> GameObject::GetTypes(ScriptHandle vm)
{
    auto typesFn = Script::Get().CreateFunction(vm, "types");

    Script::Get().EnsureSlots(vm, 1);
    wrenGetVariable((WrenVM*)vm, "gameobject", "GameObject", 0);

    auto gameObjectClass = Script::Get().GetSlotHandle(vm, 0);
    Script::Get().SetSlotHandle(vm, 0, gameObjectClass);

    Script::Get().CallFunction(vm, typesFn);
    //auto types = Script::Get().GetSlotList(vm, 0);

    return List<GameObjectType>{};
}