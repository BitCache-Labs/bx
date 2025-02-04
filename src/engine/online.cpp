#include <engine/online.hpp>
#include <engine/module.hpp>
#include <engine/script.hpp>

static const StringView g_onlineSrc = R"(
class Online {
    //foreign static test()
}
//Audio.test()
)";

BX_SCRIPT_API_REGISTRATION(Online)
{
    Script::Get().BeginModule("online");
    {
        Script::Get().BeginClass("Online");
        {
            //Script::Get().BindFunction(true, "test()", [](ScriptHandle vm) { Audio::Get().Test(); });
        }
        Script::Get().EndClass();
    }
    Script::Get().EndModule();

    ScriptModuleSource src{};
    src.moduleName = "online";
    src.moduleSource = g_onlineSrc;
    return src;
}

class NoOnline final : public Online
{
    BX_MODULE(NoOnline, Online)

public:
    bool Initialize() override;
    void Shutdown() override;

    void Update() override;
};

BX_MODULE_DEFINE(NoOnline)
BX_MODULE_DEFINE_INTERFACE(Online, NoOnline)

bool NoOnline::Initialize()
{
    return true;
}

void NoOnline::Shutdown()
{
}

void NoOnline::Update()
{
}