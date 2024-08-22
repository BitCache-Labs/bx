#include "bx/framework/systems/renderer.hpp"

#include "bx/framework/components/transform.hpp"
#include "bx/framework/components/camera.hpp"
#include "bx/framework/components/mesh_filter.hpp"
#include "bx/framework/components/mesh_renderer.hpp"
#include "bx/framework/components/animator.hpp"
#include "bx/framework/components/light.hpp"

#include "bx/framework/systems/renderer/id_pass.hpp"
#include "bx/framework/systems/renderer/present_pass.hpp"
#include "bx/framework/systems/renderer/srgb_to_linear_pass.hpp"
#include "bx/framework/systems/renderer/wfpt_pass.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/core/data.hpp>
#include <bx/engine/core/profiler.hpp>
#include <bx/engine/core/resource.hpp>
#include <bx/engine/containers/tree.hpp>
#include <bx/engine/modules/graphics.hpp>
#include <bx/engine/modules/window.hpp>

struct RendererState : NoCopy
{
    TextureHandle colorTarget = TextureHandle::null;

    TlasHandle tlas = TlasHandle::null;

    List<Camera> cameras{};

    ComputePipelineHandle pathTracerPipeline = ComputePipelineHandle::null;
};
static std::unique_ptr<RendererState> s = nullptr;

void UpdateAnimators()
{
    EntityManager::ForEach<Animator>(
        [&](Entity entity, Animator& anim)
        {
            anim.Update();
        });
}

void Renderer::UpdateCameras()
{
    s->cameras.clear();

    EntityManager::ForEach<Transform, Camera>(
        [&](Entity entity, const Transform& trx, Camera& camera)
        {
            // TODO: this shit nasty, just call camera.update()?
            i32 width, height;
            Window::GetSize(&width, &height);
            camera.SetAspect((f32)width / (height == 0 ? 1.0f : (f32)height));
            Vec3 fwd = trx.GetRotation() * Vec3::Forward();
            camera.SetView(Mat4::LookAt(trx.GetPosition(), trx.GetPosition() + fwd, Vec3::Up()));
            camera.Update();

            s->cameras.push_back(camera);
        });

    if (editorCamera.IsSome())
    {
        Camera& camera = editorCamera.Unwrap();
        s->cameras.push_back(camera);
    }
}

void UpdateTlas()
{
    List<BlasInstance> blasInstances{};

    EntityManager::ForEach<Transform, MeshFilter, MeshRenderer>(
        [&](Entity entity, const Transform& trx, const MeshFilter& mf, const MeshRenderer& mr)
        {
            if (mr.GetMaterialCount() == 0)
                return;

            SizeType index = 0;
            for (const auto& mesh : mf.GetMeshes())
            {
                const auto& material = mr.GetMaterial(index++);
                index %= mr.GetMaterialCount();

                if (!mesh || !material)
                    continue;

                BlasInstance blasInstance{};
                blasInstance.transform = trx.GetMatrix() * mesh->GetMatrix();
                blasInstance.instanceCustomIndex = 0; // TODO
                blasInstance.mask = 0xFF;
                blasInstance.blas = mesh->GetBlas();
                blasInstances.push_back(blasInstance);
            }
        });

    if (!blasInstances.empty()) // TODO: handle empty tlases internally
    {
        TlasCreateInfo tlasCreateInfo{};
        tlasCreateInfo.name = "Dynamic Tlas";
        tlasCreateInfo.blasInstances = blasInstances;
        if (s->tlas) Graphics::DestroyTlas(s->tlas);
        s->tlas = Graphics::CreateTlas(tlasCreateInfo);
    }
}

void RecreateRenderTargets()
{
    if (Window::WasResized())
    {
        i32 w, h;
        Window::GetSize(&w, &h);

        TextureCreateInfo colorTargetCreateInfo{};
        colorTargetCreateInfo.name = "Color Target";
        colorTargetCreateInfo.size = Extend3D(w, h, 1);
        colorTargetCreateInfo.format = TextureFormat::RGBA32_FLOAT;
        colorTargetCreateInfo.usageFlags = TextureUsageFlags::RENDER_ATTACHMENT | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::STORAGE_BINDING;
        if (s->colorTarget) Graphics::DestroyTexture(s->colorTarget);
        s->colorTarget = Graphics::CreateTexture(colorTargetCreateInfo);
    }
}

void Renderer::Initialize()
{
    s = std::unique_ptr<RendererState>(new RendererState());

    RecreateRenderTargets();
}

void Renderer::Shutdown()
{
    IdPass::ClearPipelineCache();
    PresentPass::ClearPipelineCache();
    SrgbToLinearPass::ClearPipelineCache();

    s.reset();
}

void Renderer::Update()
{
    RecreateRenderTargets();
}

void Renderer::Render()
{
    // TODO: this is a better fit for the update method, however, Graphics::Update is called BEFORE all the world does its updating, leaving its state 1 frame behind
    UpdateAnimators();
    UpdateCameras();
    UpdateTlas();

    TextureViewHandle colorTargetView = Graphics::CreateTextureView(s->colorTarget);
    auto& colorTargetCreateInfo = Graphics::GetTextureCreateInfo(s->colorTarget);
    u32 width = colorTargetCreateInfo.size.width;
    u32 height = colorTargetCreateInfo.size.height;

    WfptCreateInfo wfptCreateInfo{};
    wfptCreateInfo.colorTarget = s->colorTarget;
    wfptCreateInfo.tlas = s->tlas;
    WfptPass wfptPass(wfptCreateInfo);
    wfptPass.Dispatch(s->cameras.back());

    PresentPass presentPass(s->colorTarget);
    presentPass.Dispatch();
}

TextureHandle Renderer::GetEditorCameraColorTarget()
{
#ifdef BX_EDITOR_BUILD
    return s->colorTarget;// Graphics::GetSwapchainColorTarget();
#else
    return TextureHandle::null;
#endif
}