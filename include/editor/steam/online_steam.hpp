#include <editor/editor.hpp>

class OnlineSteamEditor final : public EditorWindow
{
	RTTR_ENABLE(EditorWindow)

public:
	static void ShowWindow();

public:
	OnlineSteamEditor();
	void OnGui() override;
};