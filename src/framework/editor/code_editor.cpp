#include <framework/editor/code_editor.hpp>

#include <engine/time.hpp>
#include <engine/enum.hpp>
#include <engine/window.hpp>
#include <engine/debug.hpp>

BX_EDITOR_MENUITEM_REGISTRATION("Assets/Create/Code", CodeEditorWindow)
{
    Editor::Get().AddWindow<CodeEditorWindow>();
}

CodeEditorWindow::CodeEditorWindow() {
    SetTitle("Code Editor");
    // Initialize with some default code.
    m_codeBuffer = "// Write your code here...\n\nint main() {\n    return 0;\n}\n";
}

void CodeEditorWindow::OnGui(EditorApplication& app) {
    // Determine the available space for the editor.
    ImVec2 editorSize = ImGui::GetContentRegionAvail();
    // Ensure a minimum height for the editor.
    if (editorSize.y < 200.0f)
        editorSize.y = 200.0f;

    // Render a multi-line text input widget.
    // The "##CodeEditor" label uses a hidden label to avoid visual clutter.
    ImGui::InputTextMultiline("##CodeEditor", &m_codeBuffer, editorSize, ImGuiInputTextFlags_AllowTabInput);

}