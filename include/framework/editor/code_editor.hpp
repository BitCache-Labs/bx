#pragma once

#include <framework/world.hpp>
#include <framework/camera.hpp>

#include <engine/api.hpp>
#include <engine/graphics.hpp>

#include <editor/editor.hpp>
#include <editor/assets.hpp>

struct Code {};

// A basic code editor window derived from EditorWindow.
class CodeEditorWindow : public EditorWindow {
public:
    CodeEditorWindow();
    virtual ~CodeEditorWindow() = default;

    // Called every frame to render the GUI.
    virtual void OnGui(EditorApplication& app) override;

    static void OnAssetContextMenuGui(EditorApplication& app, AssetsEditor& assets);
    static void OnAssetImport(EditorApplication& app, AssetsEditor& assets);

private:
    std::string m_codeBuffer;  // Holds the code text.
};
