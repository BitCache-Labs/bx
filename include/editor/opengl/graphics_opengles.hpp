#include <editor/editor.hpp>

class GraphicsOpenGLESEditor final : public EditorView
{
public:
	GraphicsOpenGLESEditor();
	void OnGui() override;
};