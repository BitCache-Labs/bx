#include <engine/audio.hpp>
#include <engine/script.hpp>

const StringView g_testSrc = "class Audio { foreign static test() }";

SCRIPT_API_REGISTRATION(Audio)
{
	Script::Get().BeginModule("audio");
	{
		Script::Get().BeginClass("Audio");
		{
			Script::Get().BindFunction(true, "test()", [](ScriptHandle vm) { Audio::Get().Test(); });
		}
		Script::Get().EndClass();
	}
	Script::Get().EndModule();

	Script::Get().CompileString("audio", g_testSrc);
}