#include <editor/portaudio/audio_portaudio.hpp>

BX_EDITOR_MENUITEM_REGISTRATION(ICON_EDITOR"/Modules/Audio", AudioPortAudioEditor)
{
	Editor::Get().AddWindow<AudioPortAudioEditor>();
}

AudioPortAudioEditor::AudioPortAudioEditor()
{
	SetTitle("PortAudio");
	SetExclusive(true);
	SetPresistent(false);
}

void AudioPortAudioEditor::OnGui(EditorApplication& app)
{
}