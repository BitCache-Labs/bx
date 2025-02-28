#include <engine/ecs.hpp>
#include <engine/script.hpp>

static const StringView g_ecsSrc = R"(
foreign class Entity {
    construct new(id) { }

    foreign static create()

    foreign static invalid
    
    foreign isValid
    //foreign getComponentMask()
    //foreign hasComponents()
    foreign hasComponent(cmp)
    foreign addComponent(cmp)
    foreign getComponent(cmp)
    foreign removeComponent(cmp)
    foreign destroy()
}
)";

BX_SCRIPT_API_REGISTRATION(Ecs)
{
    Script::Get().BeginModule("ecs");
    {
        Script::Get().BeginClass<u32>("Entity");
        {
            Script::Get().BindFunction(true, "create()", [](ScriptHandle vm) {});
            Script::Get().BindFunction(true, "invalid", [](ScriptHandle vm) {});
            Script::Get().BindFunction(false, "isValid", [](ScriptHandle vm) {});
            Script::Get().BindFunction(false, "hasComponent(_)", [](ScriptHandle vm) {});
            Script::Get().BindFunction(false, "addComponent(_)", [](ScriptHandle vm) {});
            Script::Get().BindFunction(false, "getComponent(_)", [](ScriptHandle vm) {});
            Script::Get().BindFunction(false, "removeComponent(_)", [](ScriptHandle vm) {});
            Script::Get().BindFunction(false, "destroy()", [](ScriptHandle vm) {});
        }
        Script::Get().EndClass();
    }
    Script::Get().EndModule();

    ScriptModuleSource src{};
    src.moduleName = "ecs";
    src.moduleSource = g_ecsSrc;
    return src;
}

// We put this in the TU to ensure ComponentBase is pure virtual
ComponentBase::~ComponentBase() {}