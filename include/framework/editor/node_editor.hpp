#pragma once

#include <framework/world.hpp>
#include <framework/camera.hpp>

#include <engine/api.hpp>
#include <engine/graphics.hpp>

#include <editor/editor.hpp>
#include <editor/assets.hpp>

struct Node {};

// A simple node editor window derived from EditorWindow.
class NodeEditorWindow : public EditorWindow {
public:
    NodeEditorWindow();
    virtual ~NodeEditorWindow() = default;

    // Called every frame to render the GUI.
    virtual void OnGui(EditorApplication& app) override;

    static void OnAssetContextMenuGui(EditorApplication& app, AssetsEditor& assets);
    static void OnAssetImport(EditorApplication& app, AssetsEditor& assets);

private:
    // A simple structure to represent a node.
    struct Node {
        std::string title;
        ImVec2 pos;       // Position relative to the canvas.
        ImVec2 size;      // Dimensions of the node.
        bool   isDragging;
        ImVec2 dragOffset;
    };

    std::vector<Node> nodes;
};