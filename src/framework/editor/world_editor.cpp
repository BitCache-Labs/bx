#include <framework/world.hpp>

#include <engine/api.hpp>
#include <editor/editor.hpp>

class BX_API WorldWEditor final
	: public EditorWindow
{
	BX_TYPE(WorldWEditor, EditorWindow)

public:
	WorldWEditor();
	void OnGui() override;

private:
	World m_world{};
};

BX_EDITOR_MENUITEM_REGISTRATION("Scene/World", WorldWEditor)
{
	Editor::Get().AddWindow<WorldWEditor>();
}

WorldWEditor::WorldWEditor()
{
	SetTitle("World");
	SetExclusive(false);
	SetPresistent(false);
}

void WorldWEditor::OnGui()
{
	m_world.Update();
	m_world.Render();
}