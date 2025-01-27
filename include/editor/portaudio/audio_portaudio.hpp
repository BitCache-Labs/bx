#pragma once

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API AudioPortAudioEditor final
	: public EditorWindow
{
	BX_TYPE(AudioPortAudioEditor, EditorWindow)

public:
	static void ShowWindow();

public:
	AudioPortAudioEditor();
	void OnGui() override;
};