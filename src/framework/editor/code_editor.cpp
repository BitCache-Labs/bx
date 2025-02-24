#include <framework/editor/code_editor.hpp>

#include <engine/time.hpp>
#include <engine/enum.hpp>
#include <engine/window.hpp>
#include <engine/debug.hpp>
#include <engine/log.hpp>

BX_ASSET_REGISTRATION(Code)
{
    AssetEditorInfo info{};
    info.onContextMenuGui = CodeEditorWindow::OnAssetContextMenuGui;
    info.onImport = CodeEditorWindow::OnAssetImport;
    return info;
}

CodeEditorWindow::CodeEditorWindow()
{
    SetTitle("Code Editor");
    // Initialize with some default code.
    m_codeBuffer = "// Write your code here...\n\nint main() {\n    return 0;\n}\n";
}

void CodeEditorWindow::OnGui(EditorApplication& app)
{
    // Determine the available space for the editor.
    ImVec2 editorSize = ImGui::GetContentRegionAvail();
    // Ensure a minimum height for the editor.
    if (editorSize.y < 200.0f)
        editorSize.y = 200.0f;

    // Render a multi-line text input widget.
    // The "##CodeEditor" label uses a hidden label to avoid visual clutter.
    ImGui::InputTextMultiline("##CodeEditor", &m_codeBuffer, editorSize, ImGuiInputTextFlags_AllowTabInput);
}

void CodeEditorWindow::OnAssetContextMenuGui(EditorApplication& app, AssetsEditor& assets)
{
    if (ImGui::MenuItem("Code"))
        Editor::Get().AddWindow<CodeEditorWindow>();
}

void CodeEditorWindow::OnAssetImport(EditorApplication& app, AssetsEditor& assets)
{
}