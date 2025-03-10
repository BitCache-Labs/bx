#include <engine/audio.hpp>
#include <engine/script.hpp>

static const StringView g_audioSrc = R"(
class Audio {
    //foreign static test()
}
//Audio.test()
)";

BX_SCRIPT_API_REGISTRATION(Audio)
{
	auto& script = Script::Get();
	script.BeginModule("audio");
	{
		script.BeginClass<Audio>("Audio");
		{
			//script.BindFunction(true, "test()", [](ScriptHandle vm) { Audio::Get().Test(); });
		}
		script.EndClass();
	}
	script.EndModule();

	ScriptModuleSource src{};
	src.moduleName = "audio";
	src.moduleSource = g_audioSrc;
	return src;
}