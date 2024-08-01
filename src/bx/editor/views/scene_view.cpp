#include "bx/editor/views/scene_view.hpp"

#include "bx/editor/core/selection.hpp"
#include "bx/editor/gizmos/transform_gizmo.hpp"

#include <bx/engine/core/input.hpp>
#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/time.hpp>
#include <bx/engine/core/file.hpp>
#include <bx/engine/core/data.hpp>
#include <bx/engine/core/inspector.hpp>
#include <bx/engine/modules/window.hpp>
#include <bx/engine/modules/graphics.hpp>
#include <bx/engine/modules/physics.hpp>

#ifdef BX_GRAPHICS_OPENGL_BACKEND
#include <bx/engine/modules/graphics/backend/graphics_opengl.hpp>
#endif
#include <bx/framework/systems/renderer/id_pass.hpp>

#include <bx/framework/components/transform.hpp>
#include <bx/framework/components/camera.hpp>
#include <bx/framework/components/mesh_filter.hpp>
#include <bx/framework/components/mesh_renderer.hpp>
#include <bx/framework/components/animator.hpp>
#include <bx/framework/components/collider.hpp>
#include <bx/framework/components/rigidbody.hpp>
#include <bx/framework/components/character_controller.hpp>
#include <bx/framework/systems/renderer.hpp>
#include <bx/framework/systems/dynamics.hpp>
#include <bx/framework/systems/acoustics.hpp>
#include <bx/framework/gameobject.hpp>

#include <cstring>
#include <fstream>
#include <sstream>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>
#include <IconsFontAwesome5.h>

#define SCENE_PAYLOAD_HEADER "SCEN"

static Vec2 s_mousePos;
static Transform g_sceneTrx;
static Camera g_sceneCam;
static Vec3 g_eulerAngles;

static ImVec2 g_sceneSize;

static bool g_physicsDebugDraw = false;

TextureHandle g_idColorTarget = TextureHandle::null;
TextureHandle g_idDepthTarget = TextureHandle::null;

void SceneView::Initialize()
{
    f32 cpx = Data::GetFloat("Scene View Cam Pos X", 0, DataTarget::EDITOR);
    f32 cpy = Data::GetFloat("Scene View Cam Pos Y", 0, DataTarget::EDITOR);
    f32 cpz = Data::GetFloat("Scene View Cam Pos Z", 0, DataTarget::EDITOR);

    g_eulerAngles.x = Data::GetFloat("Scene View Cam Rot X", 0, DataTarget::EDITOR);
    g_eulerAngles.y = Data::GetFloat("Scene View Cam Rot Y", 0, DataTarget::EDITOR);
    g_eulerAngles.z = Data::GetFloat("Scene View Cam Rot Z", 0, DataTarget::EDITOR);

    g_sceneTrx.SetPosition(Vec3(cpx, cpy, cpz));
    g_sceneTrx.SetRotation(Quat::Euler(g_eulerAngles.x, g_eulerAngles.y, g_eulerAngles.z));
}

void SceneView::Shutdown()
{
    const auto& cp = g_sceneTrx.GetPosition();

    Data::SetFloat("Scene View Cam Pos X", cp.x, DataTarget::EDITOR);
    Data::SetFloat("Scene View Cam Pos Y", cp.y, DataTarget::EDITOR);
    Data::SetFloat("Scene View Cam Pos Z", cp.z, DataTarget::EDITOR);

    Data::SetFloat("Scene View Cam Rot X", g_eulerAngles.x, DataTarget::EDITOR);
    Data::SetFloat("Scene View Cam Rot Y", g_eulerAngles.y, DataTarget::EDITOR);
    Data::SetFloat("Scene View Cam Rot Z", g_eulerAngles.z, DataTarget::EDITOR);
}

static void Update(const ImVec2& size)
{
    Vec2 mousePos = Vec2(Input::GetMouseX(), Input::GetMouseY());
    f32 dx = s_mousePos.x - mousePos.x;
    f32 dy = s_mousePos.y - mousePos.y;
    s_mousePos = mousePos;

    if (Input::GetMouseButton(MouseButton::MOUSE_BUTTON_RIGHT))
    {
        f32 s = Data::GetFloat("Scene View Cam Look Speed", 0.5f, DataTarget::EDITOR);
        g_eulerAngles += Vec3(-dy * s, dx * s, 0);
        g_eulerAngles.x = Math::Clamp(g_eulerAngles.x, -89.0f, 89.0f);
        g_sceneTrx.SetRotation(Quat::Euler(g_eulerAngles.x, g_eulerAngles.y, g_eulerAngles.z));

        Vec3 move = Vec3(Input::GetAxis(GamepadAxis::STICK_LEFT_X), 0, -Input::GetAxis(GamepadAxis::STICK_LEFT_Y));
        if (Input::GetKey(Key::W))
            move.z = 1.0f;
        if (Input::GetKey(Key::S))
            move.z = -1.0f;
        if (Input::GetKey(Key::Q))
            move.y = -1.0f;
        if (Input::GetKey(Key::E))
            move.y = 1.0f;
        if (Input::GetKey(Key::D))
            move.x = -1.0f;
        if (Input::GetKey(Key::A))
            move.x = 1.0f;

        f32 lsq = move.SqrMagnitude();
        if (lsq > 1)
            Vec3::Normalize(move);

        f32 speed = Data::GetFloat("Scene View Cam Move Speed", 50.f, DataTarget::EDITOR);
        f32 dz = Data::GetFloat("Scene View Cam Input Dead Zone", 0.1f, DataTarget::EDITOR);
        speed = lsq > dz ? speed : 0;
        move = move * speed * Time::GetDeltaTime();

        move = g_sceneTrx.GetRotation() * move;
        g_sceneTrx.SetPosition(g_sceneTrx.GetPosition() + move);
    }

    g_sceneTrx.Update();
    g_sceneCam.SetView(Mat4::LookAt(g_sceneTrx.GetPosition(), g_sceneTrx.GetPosition() + (g_sceneTrx.GetRotation() * Vec3::Forward()), Vec3::Up()));
}

void SceneView::Render(const ImVec2& size)
{
    if (g_sceneSize.x != size.x || g_sceneSize.y != size.y)
    {
        g_sceneSize = size;
        g_sceneSize.x = Math::Max(1.f, g_sceneSize.x);
        g_sceneSize.y = Math::Max(1.f, g_sceneSize.y);

        g_sceneCam.SetFov(60);
        g_sceneCam.SetAspect(g_sceneSize.x / g_sceneSize.y);
        g_sceneCam.SetZNear(0.1f);
        g_sceneCam.SetZFar(1000.0f);

        TextureCreateInfo idColorTargetCreateInfo{};
        idColorTargetCreateInfo.name = "Id Pass Color Target";
        idColorTargetCreateInfo.size = Extend3D(g_sceneSize.x, g_sceneSize.y, 1);
        idColorTargetCreateInfo.format = TextureFormat::RG32_UINT;
        idColorTargetCreateInfo.usageFlags = TextureUsageFlags::RENDER_ATTACHMENT;
        if (g_idColorTarget) Graphics::DestroyTexture(g_idColorTarget);
        g_idColorTarget = Graphics::CreateTexture(idColorTargetCreateInfo);

        TextureCreateInfo idDepthTargetCreateInfo{};
        idDepthTargetCreateInfo.name = "Id Pass Depth Target";
        idDepthTargetCreateInfo.size = Extend3D(g_sceneSize.x, g_sceneSize.y, 1);
        idDepthTargetCreateInfo.format = TextureFormat::DEPTH24_PLUS_STENCIL8;
        idDepthTargetCreateInfo.usageFlags = TextureUsageFlags::RENDER_ATTACHMENT;
        if (g_idDepthTarget) Graphics::DestroyTexture(g_idDepthTarget);
        g_idDepthTarget = Graphics::CreateTexture(idDepthTargetCreateInfo);
    }

    g_sceneCam.Update();

    EntityManager::ForEach<Transform>(
        [&](Entity entity, Transform& trx)
        {
            trx.Update();
        });

    EntityManager::ForEach<Transform, Collider>(
        [&](Entity entity, Transform& trx, Collider& coll)
        {
            bool hasRigidBody = entity.HasComponent<RigidBody>();
            coll.Build(!hasRigidBody, trx.GetMatrix());

            if (hasRigidBody)
            {
                auto& rb = entity.GetComponent<RigidBody>();
                if (rb.GetCollider() != coll.GetCollider())
                {
                    rb.SetCollider(coll.GetCollider());
                }

                rb.Build(trx.GetMatrix());
                Physics::SetRigidBodyMatrix(rb.GetRigidBody(), trx.GetMatrix());
            }

            Physics::SetColliderMatrix(coll.GetCollider(), trx.GetMatrix());
        });

    EntityManager::ForEach<Transform, CharacterController>(
        [&](Entity entity, Transform& trx, CharacterController& cc)
        {
            cc.Build(trx.GetMatrix());
            Physics::SetCharacterControllerMatrix(cc.GetCharacterController(), trx.GetMatrix());
        });

    Renderer& renderer = SystemManager::GetSystem<Renderer>();
    renderer.editorCamera = OptionalView<Camera>::Some(&g_sceneCam);
    renderer.Render();
    renderer.editorCamera = OptionalView<Camera>::None();

    IdPass idPass(g_idColorTarget, g_idDepthTarget);
    idPass.Dispatch(g_sceneCam);
}

static void CopySceneClipboard()
{
    auto selected = Selection::GetSelected();
    if (!selected.Is<Entity>())
        return;

    EntityId id = selected.As<Entity>()->GetId();

    static char buff[21];
    std::sprintf(buff, "%s%016llx", SCENE_PAYLOAD_HEADER, id);

    ImGui::SetClipboardText(buff);
}

static void PasteSceneClipboard()
{
    const char* clipboard = ImGui::GetClipboardText();
    if (clipboard == nullptr || strlen(clipboard) < 20)
        return;

    static char buff[21];
    std::strncpy(buff, clipboard, 20);
    buff[20] = '\0';

    static char header[5];
    std::strncpy(header, buff, 4);
    header[4] = '\0';
    if (std::strcmp(header, SCENE_PAYLOAD_HEADER) != 0)
        return;

    try
    {
        EntityId id = INVALID_ENTITY_ID;
        id = std::stoull(std::string(buff + 4, 16), nullptr, 16);
        if (id != INVALID_ENTITY_ID)
        {
            const auto& copy = GameObject::Duplicate(GameObject::Find(Scene::GetCurrent(), id));
            Selection::SetSelected(Object<Entity>(copy.GetEntity()));
        }
    }
    catch (...) {}
}

static void LoadGameObject(const char* path)
{
    if (path == nullptr)
        return;

    auto& obj = GameObject::Load(Scene::GetCurrent(), path);
    if (obj.GetEntity().HasComponent<Transform>())
    {
        auto& trx = obj.GetEntity().GetComponent<Transform>();
        trx.SetPosition(g_sceneTrx.GetPosition() + g_sceneTrx.GetRotation() * Vec3(0, 0, 5));
    }
}

void SceneView::Present(bool& show)
{
    ObjectRef selected = Selection::GetSelected();
    EntityId deletedId = INVALID_ENTITY_ID;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(ICON_FA_LIST"  Scene", &show, ImGuiWindowFlags_NoDecoration);
    ImGui::PopStyleVar(1);

    if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->ContentRegionRect, ImGui::GetCurrentWindow()->ID))
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("gameobject"))
        {
            auto path = (const char*)payload->Data;
            LoadGameObject(path);
        }

        ImGui::EndDragDropTarget();
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
    ImVec2 gizmoPos = ImVec2(windowPos.x + cursorPos.x, windowPos.y + cursorPos.y);

    Render(contentRegionAvail);
#ifdef BX_GRAPHICS_VULKAN_BACKEND
    // TODO: ???
    //ImGui::Image((void*)(intptr_t)GraphicsOpenGL::GetTextureHandle(g_renderTarget), contentRegionAvail, ImVec2(0, 1), ImVec2(1, 0));
#elif defined BX_GRAPHICS_OPENGL_BACKEND
    TextureHandle editorCameraColorTarget = Renderer::GetEditorCameraColorTarget();
    if (editorCameraColorTarget)
    {
        GLuint rawEditorCameraColorTarget = GraphicsOpenGL::GetRawTextureHandle(editorCameraColorTarget);
        ImGui::Image((void*)(intptr_t)rawEditorCameraColorTarget, contentRegionAvail, ImVec2(0, 1), ImVec2(1, 0));
    }
#endif
    ImGui::SetCursorScreenPos(gizmoPos);

    const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
    ImGui::Text("Current Scene: %s", scene.c_str());

    ImGui::Text("Physics Debug Draw: ");
    ImGui::SameLine();
    if (ImGui::Checkbox("##PhysicsDebugDraw", &g_physicsDebugDraw))
        Physics::SetDebugDraw(g_physicsDebugDraw);
    
    bool sceneActive = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) || ImGui::IsWindowHovered();
    if (sceneActive)
    {
        Update(contentRegionAvail);
    }

    // Use transform gizmo if selected
    bool usingGizmo = false;
    if (selected.Is<Entity>())
    {
        Entity entity = *selected.As<Entity>();
        if (entity.IsValid())
        {
            auto& trx = entity.GetComponent<Transform>();

            f32 rect[] = { gizmoPos.x, gizmoPos.y, contentRegionAvail.x, contentRegionAvail.y };
            usingGizmo = TransformGizmo::Edit(rect, g_sceneCam, trx, true, sceneActive && !ImGui::IsMouseDown(ImGuiMouseButton_Right));
        }
    }

    if (!usingGizmo && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) // Left mouse button
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 relMousePos = ImVec2(mousePos.x - windowPos.x, windowPos.y + windowSize.y - mousePos.y);
        
        u64 pixelData = 0;
        Graphics::ReadTexture(g_idColorTarget, &pixelData, Extend3D((u32)relMousePos.x, (u32)relMousePos.y, 0), Extend3D(1, 1, 1));
        if (pixelData != 0)
        {
            Selection::SetSelected(Object<Entity>(pixelData));
        }
        else
        {
            Selection::ClearSelection();
        }
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_C))
        {
            CopySceneClipboard();
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_V))
        {
            PasteSceneClipboard();
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
            Scene::Save(scene);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            if (Selection::GetSelected().Is<Entity>())
            {
                deletedId = Selection::GetSelected().As<Entity>()->GetId();
            }
        }
    }
    
    ImGui::End();

	ImGui::Begin("Hierarchy", &show, ImGuiWindowFlags_NoCollapse);
    ImGui::BeginChild("panel", ImVec2(0, 0), true);

    if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->ContentRegionRect, ImGui::GetCurrentWindow()->ID))
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("gameobject"))
        {
            auto path = (const char*)payload->Data;
            LoadGameObject(path);
        }

        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemClicked())
    {
        Selection::ClearSelection();
    }

    for (const auto gameObj : Scene::GetCurrent().GetGameObjects())
    {
        bool isParent = false; // TODO
        bool isSelected = selected.Is<Entity>() ? gameObj->GetEntity() == *selected.As<Entity>() : false;

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth
            | (!isParent ? ImGuiTreeNodeFlags_Leaf : 0)
            | (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

        EntityId entityId = gameObj->GetEntity().GetId();
        ImGui::PushID((int)entityId);
        if (ImGui::TreeNodeEx(gameObj->GetName().c_str(), flags))
        {
            if (ImGui::IsItemClicked())
            {
                Selection::SetSelected(Object<Entity>(gameObj->GetEntity()));
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Duplicate"))
                {
                    GameObject::Duplicate(*gameObj);
                }

                if (ImGui::MenuItem("Delete"))
                {
                    deletedId = gameObj->GetEntity().GetId();
                }

                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("gameobject", &entityId, sizeof(EntityId), ImGuiCond_Once);
            ImGui::Text(gameObj->GetName().c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
	ImGui::End();

    if (deletedId != INVALID_ENTITY_ID)
    {
        if (selected.Is<Entity>() && selected.As<Entity>()->GetId() == deletedId)
        {
            Selection::ClearSelection();
        }

        Entity(deletedId).Destroy();
    }
}