#include <editor/editor.hpp>

class GraphicsOpenGLEditor final : public EditorWindow
{
public:
	static void ShowWindow();

public:
	GraphicsOpenGLEditor();
	void OnGui() override;
};
