#include <framework/gameobject.hpp>
#include <engine/script.hpp>

static const StringView g_gameObjSrc = R"(
import "ecs" for Entity

foreign class GameObjectData {
    construct new() {}
    foreign type
    foreign static create(type)
    foreign static load(filepath)
}

foreign class GameObjectBase {
    construct new(data, obj) {}
    foreign initialize(data)
    foreign destroy()
    foreign entity
    foreign name
    foreign name=(v)
}

class GameObject {
    construct new(data) {
        _base = GameObjectBase.new(data, this)
    }
    initialize(data) { _base.initialize(data) }
    destroy() { _base.destroy() }

    // Bindings
    name { _base.name }
    name=(v) { _base.name = v }

    isValid { _base.entity.isValid }
    hasComponent(cmp) { _base.entity.hasComponent(cmp) }
    addComponent(cmp) { _base.entity.addComponent(cmp) }
    getComponent(cmp) { _base.entity.getComponent(cmp) }
    removeComponent(cmp) { _base.entity.removeComponent(cmp) }

    // Entry points for game logic
    start() {}
    update() {}

    // Public static functions
    foreign static register(type)

    static create(type) {
        var data = GameObjectData.create(type)
        var gameObj = type.new(data)
        gameObj.initialize(data)
        return gameObj
    }

    static load(filepath) {
        var data = GameObjectData.load(filepath)
        var gameObj = data.type.new(data)
        gameObj.initialize(data)
        return gameObj
    }
}
)";

BX_SCRIPT_API_REGISTRATION(GameObject)
{
    Script::Get().BeginModule("gameobject");
    {
        Script::Get().BeginClass("GameObject");
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