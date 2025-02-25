#pragma once

#include <framework/world.hpp>
#include <framework/camera.hpp>

#include <engine/api.hpp>
#include <engine/graphics.hpp>

#include <editor/editor.hpp>
#include <editor/assets.hpp>

struct Code
{
    String buffer{};
};

class CodeEditorWindow
    : public EditorWindow
{
    BX_TYPE(CodeEditorWindow, EditorWindow)

public:
    CodeEditorWindow();
    virtual ~CodeEditorWindow() = default;

    // Called every frame to render the GUI.
    virtual void OnGui(EditorApplication& app) override;

    static void OnAssetContextMenuGui(EditorApplication& app, AssetsEditor& assets);
    static void OnAssetImport(EditorApplication& app, AssetsEditor& assets);

private:
    BX_TYPE_REGISTRATION_FRIEND;
    
    Code m_code{};
};
