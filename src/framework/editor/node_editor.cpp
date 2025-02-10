#include <framework/editor/node_editor.hpp>

#include <engine/time.hpp>
#include <engine/enum.hpp>
#include <engine/window.hpp>
#include <engine/debug.hpp>

BX_EDITOR_MENUITEM_REGISTRATION("Assets/Create/Node", NodeEditorWindow)
{
    Editor::Get().AddWindow<NodeEditorWindow>();
}

NodeEditorWindow::NodeEditorWindow() {
    SetTitle("Node Editor");

    // Create two sample nodes with a title, position, and size.
    nodes.push_back({ "Node A", ImVec2(50, 50), ImVec2(120, 60), false, ImVec2(0, 0) });
    nodes.push_back({ "Node B", ImVec2(300, 150), ImVec2(120, 60), false, ImVec2(0, 0) });
}

void NodeEditorWindow::OnGui(EditorApplication& app) {

    //-------------------------------------------------------------------------
    // 1. Set up a canvas for the node editor.
    //-------------------------------------------------------------------------
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();   // Top-left of canvas.
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();  // Use available region.
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

    // Draw the canvas background.
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    auto& style = ImGui::GetStyle();
    draw_list->AddRectFilled(canvas_p0, canvas_p1, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_WindowBg]));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 40));

    //-------------------------------------------------------------------------
    // 2. Draw & process nodes.
    //-------------------------------------------------------------------------
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    for (auto& node : nodes) {
        // Compute the node rectangle (position relative to the canvas).
        ImVec2 node_rect_min = ImVec2(canvas_p0.x + node.pos.x, canvas_p0.y + node.pos.y);
        ImVec2 node_rect_max = ImVec2(node_rect_min.x + node.size.x, node_rect_min.y + node.size.y);

        // Draw the node: a filled rectangle with a border and title text.
        draw_list->AddRectFilled(node_rect_min, node_rect_max, IM_COL32(100, 100, 150, 200), 4.0f);
        draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(255, 255, 255, 255), 4.0f);
        draw_list->AddText(ImVec2(node_rect_min.x + 10, node_rect_min.y + 10),
            IM_COL32(255, 255, 255, 255), node.title.c_str());

        // Simple dragging logic:
        if (!node.isDragging && mouse_down && ImGui::IsMouseHoveringRect(node_rect_min, node_rect_max)) {
            node.isDragging = true;
            node.dragOffset = ImVec2(mouse_pos.x - node_rect_min.x, mouse_pos.y - node_rect_min.y);
            // Note: Removed call to ImGui::CaptureMouseFromApp(), which doesn't exist.
        }
        if (!mouse_down)
            node.isDragging = false;
        if (node.isDragging) {
            // Update node position relative to the canvas.
            node.pos = ImVec2(mouse_pos.x - node.dragOffset.x - canvas_p0.x,
                mouse_pos.y - node.dragOffset.y - canvas_p0.y);
        }
    }

    //-------------------------------------------------------------------------
    // 3. Draw a connection between the first two nodes (if available).
    //-------------------------------------------------------------------------
    if (nodes.size() >= 2) {
        Node& nodeA = nodes[0];
        Node& nodeB = nodes[1];

        ImVec2 nodeA_rect_min = ImVec2(canvas_p0.x + nodeA.pos.x, canvas_p0.y + nodeA.pos.y);
        ImVec2 nodeA_rect_max = ImVec2(nodeA_rect_min.x + nodeA.size.x, nodeA_rect_min.y + nodeA.size.y);
        ImVec2 nodeB_rect_min = ImVec2(canvas_p0.x + nodeB.pos.x, canvas_p0.y + nodeB.pos.y);
        ImVec2 nodeB_rect_max = ImVec2(nodeB_rect_min.x + nodeB.size.x, nodeB_rect_min.y + nodeB.size.y);

        // Draw a connection from the right-center of node A to the left-center of node B.
        ImVec2 nodeA_output = ImVec2(nodeA_rect_max.x, nodeA_rect_min.y + nodeA.size.y * 0.5f);
        ImVec2 nodeB_input = ImVec2(nodeB_rect_min.x, nodeB_rect_min.y + nodeB.size.y * 0.5f);
        ImVec2 cp_offset = ImVec2(50, 0); // Control point offset for the Bezier curve.

        draw_list->AddBezierCubic(nodeA_output,
            ImVec2(nodeA_output.x + cp_offset.x, nodeA_output.y),
            ImVec2(nodeB_input.x - cp_offset.x, nodeB_input.y),
            nodeB_input,
            IM_COL32(200, 200, 100, 255),
            3.0f);
    }
}
