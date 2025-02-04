#include <framework/editor/world_editor.hpp>

#include <engine/time.hpp>
#include <engine/enum.hpp>
#include <engine/window.hpp>
#include <engine/debug.hpp>

#include <IconsFontAwesome5.h>

WorldEditor::WorldEditor(SceneManager& sceneManager)
    : m_sceneManager(sceneManager)
{
	SetTitle("World");
	SetExclusive(false);
	SetPresistent(false);

	m_world = m_sceneManager.CreateScene<World>();

    // Create shaders
    {
        static const char* vsrc =
            "layout (location = 0) in vec3 Position;\n"
            "layout (std140) uniform ConstantBuffer\n"
            "{\n"
            "   mat4 ViewProjMtx;\n"
            "};\n"
            "layout (std140) uniform ModelBuffer\n"
            "{\n"
            "   mat4 WorldMeshMtx;\n"
            "   uvec2 EntityID; \n"
            "};\n"
            "flat out uvec2 Frag_EntityID;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = ViewProjMtx * WorldMeshMtx * vec4(Position, 1.0);\n"
            "   Frag_EntityID = EntityID;\n"
            "};";

        static const char* psrc =
            "layout (location = 0) out uvec2 Out_Color;\n"
            "flat in uvec2 Frag_EntityID;\n"
            "void main()\n"
            "{\n"
            "   Out_Color = Frag_EntityID;\n"
            "};";

        ShaderInfo info;

        info.shaderType = ShaderType::VERTEX;
        info.source = vsrc;
        m_vertShader = Graphics::Get().CreateShader(info);

        info.shaderType = ShaderType::PIXEL;
        info.source = psrc;
        m_pixelShader = Graphics::Get().CreateShader(info);
    }

    // Create pipeline
    {
        PipelineInfo info;

        info.numRenderTargets = 1;
        info.renderTargetFormats[0] = Graphics::Get().GetColorBufferFormat();
        info.depthStencilFormat = Graphics::Get().GetDepthBufferFormat();

        info.topology = PipelineTopology::TRIANGLES;
        info.faceCull = PipelineFaceCull::CCW;
        info.depthEnable = true;

        LayoutElement layoutElems[] =
        {
            LayoutElement { 0, 0, 3, GraphicsValueType::FLOAT32, false, 0, 0 }
        };

        info.layoutElements = layoutElems;
        info.numElements = 1;

        info.vertShader = m_vertShader;
        info.pixelShader = m_pixelShader;

        m_pipeline = Graphics::Get().CreatePipeline(info);
    }

    // Create buffers
    {
        BufferData data{};
        BufferInfo info{};

        info.type = BufferType::UNIFORM_BUFFER;
        info.usage = BufferUsage::DYNAMIC;
        info.access = BufferAccess::WRITE;
        m_constantBuffer = Graphics::Get().CreateBuffer(info, data);

        info.type = BufferType::UNIFORM_BUFFER;
        info.usage = BufferUsage::DYNAMIC;
        info.access = BufferAccess::WRITE;
        m_modelBuffer = Graphics::Get().CreateBuffer(info, data);
    }

    // Create resources
    {
        ResourceBindingElement resourceElems[] =
        {
            ResourceBindingElement { ShaderType::VERTEX, "ConstantBuffer", 1, ResourceBindingType::UNIFORM_BUFFER, ResourceBindingAccess::STATIC },
            ResourceBindingElement { ShaderType::VERTEX, "ModelBuffer", 1, ResourceBindingType::UNIFORM_BUFFER, ResourceBindingAccess::STATIC }
        };

        ResourceBindingInfo resourceBindingInfo;
        resourceBindingInfo.resources = resourceElems;
        resourceBindingInfo.numResources = 2;

        m_resources = Graphics::Get().CreateResourceBinding(resourceBindingInfo);
        Graphics::Get().BindResource(m_resources, "ConstantBuffer", m_constantBuffer);
        Graphics::Get().BindResource(m_resources, "ModelBuffer", m_modelBuffer);
    }
}

WorldEditor::~WorldEditor()
{
    Graphics::Get().DestroyShader(m_vertShader);
    Graphics::Get().DestroyShader(m_pixelShader);
    Graphics::Get().DestroyPipeline(m_pipeline);

    Graphics::Get().DestroyBuffer(m_constantBuffer);
    Graphics::Get().DestroyBuffer(m_modelBuffer);

    Graphics::Get().DestroyResourceBinding(m_resources);
    Graphics::Get().DestroyTexture(m_renderTarget);
    Graphics::Get().DestroyTexture(m_renderTargetIDs);
    Graphics::Get().DestroyTexture(m_depthStencil);

    m_sceneManager.DestroyScene(m_world);
}

void WorldEditor::OnToolbarGui(World& world)
{
    f32 ok_timer = 0;// -= Time::GetDeltaTime();
    ImVec2 topPanelSize;

    //if (Script::HasError() || show_data || show_profiler || show_scene || show_entity || show_assets
    //	|| (ImGui::IsMousePosValid() && ImGui::GetMousePos().y < 100.0f))
    if ((!world.m_playing || world.m_paused) ||
        (world.m_playing && ImGui::IsMousePosValid() &&
            (ImGui::GetMousePos().y - ImGui::GetWindowPos().y) < 100.0f))
    {
        if (ImGui::BeginMenuBar())
        {
            topPanelSize = ImGui::GetWindowSize();

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "CTRL+N")) {}
                if (ImGui::MenuItem("Open", "CTRL+O"))
                {
                    //const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
                    //
                    //Runtime::Reload();
                    //Scene::Load(scene);
                }
                if (ImGui::MenuItem("Save", "CTRL+S"))
                {
                    //const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
                    //Scene::Save(scene);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    //Runtime::Close();
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                if (ImGui::MenuItem("Paste", "CTRL+V")) {}

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Inside your parent window code (i.e. inside an ImGui::Begin/End block)

        // Determine the available content region size:
        ImVec2 contentSize = ImGui::GetContentRegionAvail();

        // If you want to fix the toolbar height, you can set it here. For example:
        const float toolbarHeight = 30.0f; // or any value you like

        // Begin the child window that will act as the toolbar.
        // The child window inherits its parent's position and clipping.
        if (ImGui::BeginChild("##ApplicationToolbarChild",
            ImVec2(contentSize.x, toolbarHeight),
            false, // No border by default; adjust as needed
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoDocking))
        {
            // Store the current height of the toolbar child
            topPanelSize.y = ImGui::GetWindowSize().y;

            // Save current style colors if needed.
            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_WindowBg]);

            // Lay out the toolbar items on the same line.
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_ADJUST))
            {
                //theme = !theme;
                //SelectTheme();
            }
            //Tooltip("Theme");

            ImGui::SameLine();
            ImGui::Separator();

            //ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            //ImGui::SameLine();
            //
            //ImGui::Text("| Engine v%s |", BX_VERSION_STR);
            //Tooltip("Version");
            //ImGui::SameLine();
            //
            //if (ImGui::Button(ICON_FA_QUESTION_CIRCLE))
            //{
            //    // TODO: About dialog
            //}
            //Tooltip("About");
            //
            //ImGui::PopStyleColor(); // Pop text color

            ImGui::SameLine();
            ImGui::Separator();

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_BUG))
            {
                //show_settings = !show_settings;
            }
            //Tooltip("Debug");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CHART_PIE))
            {
                //show_profiler = !show_profiler;
            }
            //Tooltip("Profiler");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_DATABASE)) // ICON_FA_COG alternative
            {
                //show_data = !show_data;
            }
            //Tooltip("Data");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_TERMINAL))
            {
                //show_console = !show_console;
            }
            //Tooltip("Console");

            ImGui::SameLine();
            ImGui::Separator();

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_LIST))
            {
                //show_scene = !show_scene;
            }
            //Tooltip("Scene");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_EDIT))
            {
                //show_inspector = !show_inspector;
            }
            //Tooltip("Inspector");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_IMAGES))
            {
                //show_assets = !show_assets;
            }
            //Tooltip("Assets");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_ID_BADGE))
            {
                //show_gameobjects = !show_gameobjects;
            }
            //Tooltip("GameObjects");

            ImGui::SameLine();
            ImGui::Separator();

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_SYNC_ALT))
            {
                //const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
                //Runtime::Reload();
                //Scene::Load(scene);
            }
            //Tooltip("Reload");

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[world.m_playing ? ImGuiCol_ButtonHovered : ImGuiCol_Button]);
            if (ImGui::Button(ICON_FA_PLAY))
            {
                world.m_playing = !world.m_playing;
                if (!world.m_playing)
                {
                    world.m_paused = false;
                    //const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
                    //Runtime::Reload();
                    //Scene::Load(scene);
                    //Window::SetCursorMode(CursorMode::NORMAL);
                }
                else
                {
                    //const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
                    //Scene::Save(scene);
                    //Window::SetCursorMode(CursorMode::DISABLED);
                }
            }
            ImGui::PopStyleColor();
            //Tooltip("Play");

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[world.m_paused ? ImGuiCol_ButtonHovered : ImGuiCol_Button]);
            if (ImGui::Button(ICON_FA_PAUSE))
            {
                world.m_paused = !world.m_paused;
            }
            ImGui::PopStyleColor();
            //Tooltip("Pause");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_FAST_FORWARD))
            {
            }//next_frame = true;
            //Tooltip("Next Frame");

            ImGui::SameLine();
            ImGui::Separator();

            //ImGui::SameLine();
            //if (ok_timer > 0.0f)
            //{
            //    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 1.0f, 0.1f, 1.0f));
            //    if (ImGui::Button(ICON_FA_CHECK_CIRCLE))
            //        ok_timer = 0.0f;
            //    ImGui::PopStyleColor();
            //    Tooltip("Reload Complete");
            //}
            //
            //ImGui::SameLine();
            //if (Script::Get().HasError(world.m_vm))
            //{
            //    paused = true;
            //    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
            //    if (ImGui::Button(ICON_FA_TIMES_CIRCLE))
            //    {
            //        Script::Get().ClearError(world.m_vm);
            //        paused = false;
            //    }
            //    Tooltip("Script Error! Check output.");
            //    ImGui::PopStyleColor();
            //}

            //if (xs::data::has_chages()) {
            //	ImGui::SameLine();
            //	ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFF5FB9);
            //	if (ImGui::Button(ICON_FA_EXCLAMATION_TRIANGLE)) {
            //		show_data = true;
            //	}
            //	ImGui::PopStyleColor();
            //	Tooltip("Data has unsaved changes");
            //}

            ImGui::PopStyleColor(); // Pop the button style color pushed earlier

        } // End of child window content

        ImGui::EndChild();
    }

    /*
    if (!IsPlaying() || IsPaused())
    {
        // Setup Dockspace
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + topPanelSize.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - topPanelSize.y));
        ImGui::SetNextWindowViewport(viewport->ID);
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("##DockSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(2);

        // Dockspace
        ImGuiID dockspaceId = ImGui::GetID("##DockSpaceID");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        if (show_data)
        {
            DataView::Present(show_data);
        }

        if (show_profiler)
        {
            ProfilerView::Present(show_profiler);
        }

        if (show_inspector)
        {
            InspectorView::Present(show_inspector);
        }

        if (show_assets)
        {
            AssetsView::Present(show_assets);
        }

        if (show_gameobjects)
        {
            GameObjectView::Present(show_gameobjects);
        }

        if (show_console)
        {
            ConsoleView::Present(show_console);
        }

        //if (show_settings)
        //{
        //	SettingsView::Inspect(show_settings);
        //}

        if (show_scene)
        {
            SceneView::Present(show_scene);
        }

        ImGui::End();
    }
    else
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
        {
            Window::SetCursorMode(CursorMode::DISABLED);
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_F1))
        {
            Window::SetCursorMode(CursorMode::NORMAL);
        }
    }

    //ImGui::EndDisabled();
    */
}

void WorldEditor::OnGui(EditorApplication& app)
{
    auto& world = static_cast<World&>(*m_sceneManager.GetScene(m_world));

    OnToolbarGui(world);

    m_frames++;
    m_timer += Time::Get().DeltaTime();
    if (m_timer >= 1.f)
    {
        m_fps = (i32)(m_frames / m_timer);
        m_frames = 0;
        m_timer = Math::FMod(m_timer, 1.f);
    }

    CString<64> fps;
    fps.format("FPS: {}", m_fps);
    ImGui::Text(fps);
    ImGui::SameLine();

    //ObjectRef selected = Selection::GetSelected();
    //EntityId deletedId = INVALID_ENTITY_ID;
    //
    //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    //ImGui::Begin(ICON_FA_LIST"  Scene", &show, ImGuiWindowFlags_NoDecoration);
    //ImGui::PopStyleVar(1);

    if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->ContentRegionRect, ImGui::GetCurrentWindow()->ID))
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("gameobject"))
        {
            auto path = (const char*)payload->Data;
            //LoadGameObject(path);
        }

        ImGui::EndDragDropTarget();
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
    ImVec2 gizmoPos = ImVec2(windowPos.x + cursorPos.x, windowPos.y + cursorPos.y);

    Render(world, contentRegionAvail);
    ImGui::Image(Graphics::Get().GetImTextureID(m_renderTarget), contentRegionAvail, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::SetCursorScreenPos(gizmoPos);

    //const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
    //ImGui::Text("Current Scene: %s", scene.c_str());

    //ImGui::Text("Physics Debug Draw: ");
    //ImGui::SameLine();
    //if (ImGui::Checkbox("##PhysicsDebugDraw", &m_physicsDebugDraw))
    //    Physics::Get().SetDebugDraw(g_physicsDebugDraw);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::FocusWindow(ImGui::GetCurrentWindow());

    bool sceneActive = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);// || ImGui::IsWindowHovered();
    if (sceneActive)
    {
        Update(world);
    }

    // Use transform gizmo if selected
    //bool usingGizmo = false;
    //if (selected.Is<Entity>())
    //{
    //    Entity entity = *selected.As<Entity>();
    //    if (entity.IsValid())
    //    {
    //        auto& trx = entity.GetComponent<Transform>();
    //
    //        f32 rect[] = { gizmoPos.x, gizmoPos.y, contentRegionAvail.x, contentRegionAvail.y };
    //        usingGizmo = TransformGizmo::Edit(rect, g_sceneCam, trx, true, sceneActive && !ImGui::IsMouseDown(ImGuiMouseButton_Right));
    //    }
    //}

    //if (!usingGizmo && ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) // Left mouse button
    //{
    //    ImVec2 mousePos = ImGui::GetMousePos();
    //    ImVec2 windowPos = ImGui::GetWindowPos();
    //    ImVec2 windowSize = ImGui::GetWindowSize();
    //    ImVec2 relMousePos = ImVec2(mousePos.x - windowPos.x, windowPos.y + windowSize.y - mousePos.y);
    //
    //    u64 pixelData = 0;
    //    Graphics::Get().ReadPixels((u32)relMousePos.x, (u32)relMousePos.y, 1, 1, &pixelData, g_renderTargetIDs);
    //    if (pixelData != 0)
    //    {
    //        Selection::SetSelected(Object<Entity>(pixelData));
    //    }
    //    else
    //    {
    //        Selection::ClearSelection();
    //    }
    //}

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_C))
        {
            //CopySceneClipboard();
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_V))
        {
            //PasteSceneClipboard();
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S))
        {
            //const String& scene = Data::GetString("Current Scene", "", DataTarget::EDITOR);
            //Scene::Save(scene);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            //if (Selection::GetSelected().Is<Entity>())
            //{
            //    deletedId = Selection::GetSelected().As<Entity>()->GetId();
            //}
        }
    }

    //ImGui::End();

    //ImGui::Begin("Hierarchy", &show, ImGuiWindowFlags_NoCollapse);
    //ImGui::BeginChild("panel", ImVec2(0, 0), true);
    //
    //if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->ContentRegionRect, ImGui::GetCurrentWindow()->ID))
    //{
    //    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("gameobject"))
    //    {
    //        auto path = (const char*)payload->Data;
    //        LoadGameObject(path);
    //    }
    //
    //    ImGui::EndDragDropTarget();
    //}
    //
    //if (ImGui::IsItemClicked())
    //{
    //    Selection::ClearSelection();
    //}
    //
    //for (const auto gameObj : Scene::GetCurrent().GetGameObjects())
    //{
    //    bool isParent = false; // TODO
    //    bool isSelected = selected.Is<Entity>() ? gameObj->GetEntity() == *selected.As<Entity>() : false;
    //
    //    ImGuiTreeNodeFlags flags =
    //        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth
    //        | (!isParent ? ImGuiTreeNodeFlags_Leaf : 0)
    //        | (isSelected ? ImGuiTreeNodeFlags_Selected : 0);
    //
    //    EntityId entityId = gameObj->GetEntity().GetId();
    //    ImGui::PushID((int)entityId);
    //    if (ImGui::TreeNodeEx(gameObj->GetName().c_str(), flags))
    //    {
    //        if (ImGui::IsItemClicked())
    //        {
    //            Selection::SetSelected(Object<Entity>(gameObj->GetEntity()));
    //        }
    //
    //        if (ImGui::BeginPopupContextItem())
    //        {
    //            if (ImGui::MenuItem("Duplicate"))
    //            {
    //                GameObject::Duplicate(*gameObj);
    //            }
    //
    //            if (ImGui::MenuItem("Delete"))
    //            {
    //                deletedId = gameObj->GetEntity().GetId();
    //            }
    //
    //            ImGui::EndPopup();
    //        }
    //
    //        ImGui::TreePop();
    //    }
    //
    //    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    //    {
    //        ImGui::SetDragDropPayload("gameobject", &entityId, sizeof(EntityId), ImGuiCond_Once);
    //        ImGui::Text(gameObj->GetName().c_str());
    //        ImGui::EndDragDropSource();
    //    }
    //
    //    ImGui::PopID();
    //}
    //
    //ImGui::EndChild();
    //ImGui::End();
    //
    //if (deletedId != INVALID_ENTITY_ID)
    //{
    //    if (selected.Is<Entity>() && selected.As<Entity>()->GetId() == deletedId)
    //    {
    //        Selection::ClearSelection();
    //    }
    //
    //    Entity(deletedId).Destroy();
    //}
}

void WorldEditor::Update(World& world)
{
    // Get the ImGui IO structure for input state
    ImGuiIO& io = ImGui::GetIO();

    // Rotate camera when the right mouse button is held down
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        // Use mouse delta provided by ImGui
        m_cameraRot.x += io.MouseDelta.y * m_lookSpeed;
        m_cameraRot.y -= io.MouseDelta.x * m_lookSpeed;
    }

    // Initialize movement delta vector to zero
    Vec3 moveDelta{ 0.0f, 0.0f, 0.0f };

    // Use ImGui key queries for movement keys
    if (ImGui::IsKeyDown(ImGuiKey_W))
        moveDelta.z += 1;
    if (ImGui::IsKeyDown(ImGuiKey_S))
        moveDelta.z -= 1;
    if (ImGui::IsKeyDown(ImGuiKey_A))
        moveDelta.x += 1;
    if (ImGui::IsKeyDown(ImGuiKey_D))
        moveDelta.x -= 1;
    if (ImGui::IsKeyDown(ImGuiKey_E))
        moveDelta.y += 1;
    if (ImGui::IsKeyDown(ImGuiKey_Q))
        moveDelta.y -= 1;

    // Set move speed and boost it if SPACE is held down
    float moveSpeed = m_moveSpeed;
    if (ImGui::IsKeyDown(ImGuiKey_Space))
        moveSpeed *= 20;

    Quat camRot = Quat::Euler(m_cameraRot.x, m_cameraRot.y, m_cameraRot.z);
    m_cameraPos += camRot * moveDelta * moveSpeed * Time::Get().DeltaTime();

    // Update view data
    int w, h;
    Window::Get().GetSize(&w, &h);
    m_camera.SetProjection(Mat4::Perspective(60.f, h > 0 ? (f32)w / h : 1.f, 0.1f, 5000.f));
    m_camera.SetView(Mat4::LookAt(m_cameraPos, m_cameraPos + camRot * Vec3::Forward(), Vec3::Up()));
    m_camera.Update();

    m_constantData.viewProjMtx = m_camera.GetViewProjection();

    world.OnUpdate();
}

static void DrawXZGrid(const Vec3& cameraPos, f32 zfar)
{
    f32 cameraHeight = fabs(cameraPos.y);

    // Determine the current grid spacing based on camera height
    f32 currentSpacing = pow(10.f, floor(log10(cameraHeight)));
    f32 nextSpacing = currentSpacing * 10.f;

    // Grid extent based on zfar
    f32 gridExtent = std::min(100.0f * nextSpacing, zfar);

    // Colors
    static const u32 lightGray = 0xFF555555;
    static const u32 darkGray = 0xFF444444;
    static const u32 red = 0xFF0000FF;
    static const u32 green = 0xFF00FF00;
    static const u32 blue = 0xFFFF0000;

    // Round the camera's position to the nearest grid spacing
    f32 roundedX = currentSpacing * floor(cameraPos.x / currentSpacing);
    f32 roundedY = currentSpacing * floor(cameraPos.y / currentSpacing);
    f32 roundedZ = currentSpacing * floor(cameraPos.z / currentSpacing);

    // Draw the grid lines in X and Z directions
    for (f32 x = roundedX - gridExtent; x < roundedX + gridExtent; x += currentSpacing)
    {
        if (fabs(x - cameraPos.x) > gridExtent) continue; // Check if line is within view range
        if (fabs(x) < 0.0001f) continue; // Skip center line

        u32 colorX = fmod(fabs(x / currentSpacing), 10.0f) == 0 ? lightGray : darkGray;
        Debug::Get().DrawLine(Vec3(x, 0, roundedZ - gridExtent), Vec3(x, 0, roundedZ + gridExtent), colorX);
    }

    for (f32 z = roundedZ - gridExtent; z < roundedZ + gridExtent; z += currentSpacing)
    {
        if (fabs(z - cameraPos.z) > gridExtent) continue; // Check if line is within view range
        if (fabs(z) < 0.0001f) continue; // Skip center line

        u32 colorZ = fmod(fabs(z / currentSpacing), 10.0f) == 0 ? lightGray : darkGray;
        Debug::Get().DrawLine(Vec3(roundedX - gridExtent, 0, z), Vec3(roundedX + gridExtent, 0, z), colorZ);
    }

    // Center lines (Global origin, drawn at origin 0, 0, 0)
    if (fabs(cameraPos.y) <= gridExtent && fabs(cameraPos.z) <= gridExtent)
        Debug::Get().DrawLine(Vec3(roundedX - gridExtent, 0, 0), Vec3(roundedX + gridExtent, 0, 0), red); // X-axis
    if (fabs(cameraPos.x) <= gridExtent && fabs(cameraPos.z) <= gridExtent)
        Debug::Get().DrawLine(Vec3(0, roundedY - gridExtent, 0), Vec3(0, roundedY + gridExtent, 0), green); // Y-axis
    if (fabs(cameraPos.x) <= gridExtent && fabs(cameraPos.y) <= gridExtent)
        Debug::Get().DrawLine(Vec3(0, 0, roundedZ - gridExtent), Vec3(0, 0, roundedZ + gridExtent), blue); // Z-axis
}

void WorldEditor::Render(World& world, const ImVec2& size)
{
    if (m_screenSize.x != size.x || m_screenSize.y != size.y)
    {
        m_screenSize = size;
        m_screenSize.x = Math::Max(1.f, m_screenSize.x);
        m_screenSize.y = Math::Max(1.f, m_screenSize.y);

        m_camera.SetView(Mat4::Perspective(60.0f, m_screenSize.x / m_screenSize.y, 0.1f, 1000.0f));

        // Check if there are old render targets
        if (m_renderTargetIDs != INVALID_GRAPHICS_HANDLE)
            Graphics::Get().DestroyTexture(m_renderTargetIDs);

        if (m_renderTarget != INVALID_GRAPHICS_HANDLE)
            Graphics::Get().DestroyTexture(m_renderTarget);

        if (m_depthStencil != INVALID_GRAPHICS_HANDLE)
            Graphics::Get().DestroyTexture(m_depthStencil);

        // Create render targets
        {
            BufferData data{};

            TextureInfo info;
            info.width = (u32)m_screenSize.x;
            info.height = (u32)m_screenSize.y;

            info.format = TextureFormat::RG32_UINT;
            info.flags = TextureFlags::SHADER_RESOURCE | TextureFlags::RENDER_TARGET;
            info.enableMipmaps = false;
            m_renderTargetIDs = Graphics::Get().CreateTexture(info, data);

            info.format = TextureFormat::RGBA8_UNORM;
            info.flags = TextureFlags::SHADER_RESOURCE | TextureFlags::RENDER_TARGET;
            info.enableMipmaps = false;
            m_renderTarget = Graphics::Get().CreateTexture(info, data);

            info.format = TextureFormat::D24_UNORM_S8_UINT;
            info.flags = TextureFlags::DEPTH_STENCIL;
            info.enableMipmaps = false;
            m_depthStencil = Graphics::Get().CreateTexture(info, data);
        }
    }

    //g_sceneCam.Update();
    //
    //EntityManager::ForEach<Transform>(
    //    [&](Entity entity, Transform& trx)
    //    {
    //        trx.Update();
    //    });
    //
    //EntityManager::ForEach<Transform, Collider>(
    //    [&](Entity entity, Transform& trx, Collider& coll)
    //    {
    //        bool hasRigidBody = entity.HasComponent<RigidBody>();
    //        coll.Build(!hasRigidBody, trx.GetMatrix());
    //
    //        if (hasRigidBody)
    //        {
    //            auto& rb = entity.GetComponent<RigidBody>();
    //            if (rb.GetCollider() != coll.GetCollider())
    //            {
    //                rb.SetCollider(coll.GetCollider());
    //            }
    //
    //            rb.Build(trx.GetMatrix());
    //            Physics::SetRigidBodyMatrix(rb.GetRigidBody(), trx.GetMatrix());
    //        }
    //
    //        Physics::SetColliderMatrix(coll.GetCollider(), trx.GetMatrix());
    //    });
    //
    //EntityManager::ForEach<Transform, CharacterController>(
    //    [&](Entity entity, Transform& trx, CharacterController& cc)
    //    {
    //        cc.Build(trx.GetMatrix());
    //        Physics::SetCharacterControllerMatrix(cc.GetCharacterController(), trx.GetMatrix());
    //    });
    //
    //auto& renderer = SystemManager::GetSystem<Renderer>();
    //renderer.UpdateAnimators();
    //renderer.UpdateCameras();
    //renderer.UpdateLights();
    //renderer.CollectDrawCommands();
    //
    //g_drawCmds.clear();
    //
    //EntityManager::ForEach<Transform, MeshFilter>(
    //    [&](Entity entity, const Transform& trx, const MeshFilter& mf)
    //    {
    //        for (const auto& mesh : mf.GetMeshes())
    //        {
    //            if (!mesh) continue;
    //            const auto& meshData = mesh.GetData();
    //
    //            SceneDrawCommandData cmd;
    //            cmd.model.worldMeshMtx = trx.GetMatrix() * meshData.GetMatrix();
    //            cmd.model.entityId = entity.GetId();
    //            cmd.vbuffers = meshData.GetVertexBuffers();
    //            cmd.ibuffer = meshData.GetIndexBuffer();
    //            cmd.numIndices = static_cast<u32>(meshData.GetTriangles().size());
    //
    //            g_drawCmds.emplace_back(cmd);
    //        }
    //    });

    const f32 viewport[] = { 0.0f, 0.0f, m_screenSize.x, m_screenSize.y };
    Graphics::Get().SetViewport(viewport);

    // Render to the normal color render target
    Graphics::Get().SetRenderTarget(m_renderTarget, m_depthStencil);
    
    static f32 clearColor[] = { .2f, .2f, .2f, 1.f };
    Graphics::Get().ClearRenderTarget(m_renderTarget, clearColor);
    Graphics::Get().ClearDepthStencil(m_depthStencil, GraphicsClearFlags::DEPTH, 1.0f, 0);

    //renderer.BindConstants(g_sceneCam.GetView(), g_sceneCam.GetProjection(), g_sceneCam.GetViewProjection());
    //renderer.DrawCommands();
    //
    //Physics::DebugDraw();

    //Graphics::Get().UpdateDebugLines();
    //Graphics::Get().DrawDebugLines(g_sceneCam.GetViewProjection());

    //// Render to the ID render target
    //Graphics::Get().SetRenderTarget(m_renderTargetIDs, m_depthStencil);
    //
    //Graphics::Get().SetPipeline(m_pipeline);
    //Graphics::Get().ClearRenderTarget(m_renderTargetIDs, clearColor);
    //Graphics::Get().ClearDepthStencil(m_depthStencil, GraphicsClearFlags::DEPTH, 1.0f, 0);
    //
    //BufferData bufferData;
    //SceneConstantData constants;
    //constants.viewProjMtx = g_sceneCam.GetViewProjection();
    //bufferData.dataSize = sizeof(SceneConstantData);
    //bufferData.pData = &constants;
    //Graphics::Get().UpdateBuffer(g_constantBuffer, bufferData);
    //
    //for (auto& cmd : g_drawCmds)
    //{
    //    bufferData.dataSize = sizeof(SceneModelData);
    //    bufferData.pData = &cmd.model;
    //    Graphics::Get().UpdateBuffer(g_modelBuffer, bufferData);
    //
    //    const u64 offset = 0;
    //    renderer.DrawCommand(g_pipeline, 1, &g_resources, 1, &cmd.vbuffers, &offset, cmd.ibuffer, cmd.numIndices);
    //}

    DrawXZGrid(m_cameraPos, 5000.f);
    Debug::Get().DrawLine(Vec3(0, 0, 0), Vec3(1, 0, 1), 0xFFFFFFFF); // Z-axis

    BufferData bufferData;
    bufferData.dataSize = sizeof(ConstantData);
    bufferData.pData = &m_constantData;
    Graphics::Get().UpdateBuffer(m_constantBuffer, bufferData);

    world.OnRender();

    Debug::Get().RenderDraws(m_camera.GetViewProjection());
    Debug::Get().ClearDraws();
}