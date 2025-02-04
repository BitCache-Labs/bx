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
        Script::Get().BeginClass("Online");
        {
            //Script::Get().BindFunction(true, "test()", [](ScriptHandle vm) { Audio::Get().Test(); });
        }
        Script::Get().EndClass();
    }
    Script::Get().EndModule();

    ScriptModuleSource src{};
    src.moduleName = "ecs";
    src.moduleSource = g_ecsSrc;
    return src;
}
