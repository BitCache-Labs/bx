#include <editor/editor.hpp>

class WindowGLFWEditor final : public EditorWindow
{
public:
	static void ShowWindow();

public:
	WindowGLFWEditor();
	void OnGui() override;
};
