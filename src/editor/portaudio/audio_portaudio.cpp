#include <editor/portaudio/audio_portaudio.hpp>

EDITOR_MENUITEM("Modules/Audio/PortAudio", AudioPortAudioEditor)
void AudioPortAudioEditor::ShowWindow()
{
	Editor::Get().AddWindow<AudioPortAudioEditor>();
}

AudioPortAudioEditor::AudioPortAudioEditor()
{
	SetTitle("PortAudio");
	SetExclusive(true);
	SetPresistent(false);
}

void AudioPortAudioEditor::OnGui()
{
}