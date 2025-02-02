#pragma once

#include <engine/api.hpp>
#include <engine/graphics.hpp>
#include <editor/editor.hpp>
#include <framework/world.hpp>
#include <framework/camera.hpp>

#include <imgui_internal.h>

struct BX_API ConstantData
{
    Mat4 viewProjMtx{};
};

class BX_API WorldEditor final
    : public EditorWindow
{
    BX_TYPE(WorldEditor, EditorWindow)

public:
    WorldEditor(SceneManager& sceneManager);
    ~WorldEditor();

    void OnGui(EditorApplication& app) override;

    void Update(World& world);
    void Render(World& world, const ImVec2& size);

private:
    SceneManager& m_sceneManager;
    SceneHandle m_world{ SCENE_INVALID_HANDLE };

    f32 m_timer{ 1.f };
    i32 m_frames{ 0 };
    i32 m_fps{ 0 };

    // Render data
    ConstantData m_constantData{};

    ImVec2 m_screenSize{ 0, 0 };

    bool m_physicsDebugDraw = false;

    GraphicsHandle m_vertShader{ INVALID_GRAPHICS_HANDLE };
    GraphicsHandle m_pixelShader{ INVALID_GRAPHICS_HANDLE };
    GraphicsHandle m_pipeline{ INVALID_GRAPHICS_HANDLE };

    GraphicsHandle m_constantBuffer{ INVALID_GRAPHICS_HANDLE };
    GraphicsHandle m_modelBuffer{ INVALID_GRAPHICS_HANDLE };

    GraphicsHandle m_resources{ INVALID_GRAPHICS_HANDLE };
    GraphicsHandle m_renderTarget{ INVALID_GRAPHICS_HANDLE };
    GraphicsHandle m_renderTargetIDs{ INVALID_GRAPHICS_HANDLE };
    GraphicsHandle m_depthStencil{ INVALID_GRAPHICS_HANDLE };

    // Camera data
    Vec2 m_mousePos{ 0, 0 };
    Vec3 m_cameraPos{ 0, 5, 0 };
    Vec3 m_cameraRot{ 0, 0, 0 };
    f32 m_lookSpeed = 0.1f;
    f32 m_moveSpeed = 10.0f;
    Camera m_camera;
};