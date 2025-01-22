#include <editor/editor.hpp>

class AudioPortAudioEditor final : public EditorView
{
public:
	AudioPortAudioEditor();
	void OnGui() override;
};

EDITOR_MENU("Modules/Audio/PortAudio", []() { EditorManager::Get().AddView(meta::make_unique<AudioPortAudioEditor>()); })

AudioPortAudioEditor::AudioPortAudioEditor()
{
	SetTitle("PortAudio");
	SetExclusive(true);
	SetPresistent(false);
}

void AudioPortAudioEditor::OnGui()
{
}