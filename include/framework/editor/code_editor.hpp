#pragma once

#include <engine/api.hpp>
#include <engine/graphics.hpp>
#include <editor/editor.hpp>
#include <framework/world.hpp>
#include <framework/camera.hpp>

// A basic code editor window derived from EditorWindow.
class CodeEditorWindow : public EditorWindow {
public:
    CodeEditorWindow();
    virtual ~CodeEditorWindow() = default;

    // Called every frame to render the GUI.
    virtual void OnGui(EditorApplication& app) override;

private:
    std::string m_codeBuffer;  // Holds the code text.
};
