#include <editor/settings.hpp>

BX_EDITOR_MENUITEM_REGISTRATION("Window/Settings", SettingsEditor)
{
    Editor::Get().AddWindow<SettingsEditor>();
}

SettingsEditor::SettingsEditor()
{
    SetTitle("Settings");
    SetExclusive(true);
    SetPresistent(false);
}

void SettingsEditor::OnGui(EditorApplication& app)
{
}