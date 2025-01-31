#include <engine/audio.hpp>
#include <engine/script.hpp>

static const StringView g_audioSrc = R"(
class Audio {
    //foreign static test()
}
Audio.test()
)";

BX_SCRIPT_API_REGISTRATION(Audio)
{
	Script::Get().BeginModule("audio");
	{
		Script::Get().BeginClass("Audio");
		{
			//Script::Get().BindFunction(true, "test()", [](ScriptHandle vm) { Audio::Get().Test(); });
		}
		Script::Get().EndClass();
	}
	Script::Get().EndModule();

	ScriptModuleSource src{};
	src.moduleName = "audio";
	src.moduleSource = g_audioSrc;
	return src;
}