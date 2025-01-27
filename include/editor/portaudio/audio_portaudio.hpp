#include <editor/editor.hpp>

class AudioPortAudioEditor final : public EditorWindow
{
public:
	static void ShowWindow();

public:
	AudioPortAudioEditor();
	void OnGui() override;
};