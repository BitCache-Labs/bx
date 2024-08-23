
#include "bx/framework/systems/renderer.hpp"

#include "bx/framework/components/transform.hpp"
#include "bx/framework/components/mesh_filter.hpp"
#include "bx/framework/components/mesh_renderer.hpp"
#include "bx/framework/components/animator.hpp"
#include "bx/framework/components/light.hpp"

#include "bx/framework/systems/renderer/id_pass.hpp"
#include "bx/framework/systems/renderer/present_pass.hpp"
#include "bx/framework/systems/renderer/srgb_to_linear_pass.hpp"
#include "bx/framework/systems/renderer/write_indirect_args_pass.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/core/data.hpp>
#include <bx/engine/core/profiler.hpp>
#include <bx/engine/core/resource.hpp>
#include <bx/engine/containers/tree.hpp>
#include <bx/engine/modules/graphics.hpp>
#include <bx/engine/modules/window.hpp>

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
    m_cameras.clear();

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

            m_cameras.push_back(camera);
        });

    if (m_editorCamera.IsSome())
    {
        Camera& camera = m_editorCamera.Unwrap();
        m_cameras.push_back(camera);
    }
}

void Renderer::UpdateTlas()
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

                Mat4 matrix = trx.GetMatrix() * mesh->GetMatrix();

                m_blasDataPool->SubmitInstance(mesh.GetData(), mesh.GetHandle(), matrix.Inverse());

                BlasInstance blasInstance{};
                blasInstance.transform = matrix;
                blasInstance.instanceCustomIndex = blasInstances.size(); // TODO
                blasInstance.mask = 0xFF;
                blasInstance.blas = mesh->GetBlas();
                blasInstances.push_back(blasInstance);
            }
        });

    m_blasDataPool->Submit();

    if (!blasInstances.empty()) // TODO: handle empty tlases internally
    {
        TlasCreateInfo tlasCreateInfo{};
        tlasCreateInfo.name = "Dynamic Tlas";
        tlasCreateInfo.blasInstances = blasInstances;
        if (m_tlas) Graphics::DestroyTlas(m_tlas);
        m_tlas = Graphics::CreateTlas(tlasCreateInfo);

        if (m_wfptPass)
        {
            m_wfptPass->SetTlas(m_tlas);
        }
    }
}

void Renderer::RecreateRenderTargets()
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
        if (m_colorTarget) Graphics::DestroyTexture(m_colorTarget);
        m_colorTarget = Graphics::CreateTexture(colorTargetCreateInfo);

        m_dirtyPasses = true;
    }
}

void Renderer::RebuildPasses()
{
    if (m_dirtyPasses)
    {
        WfptCreateInfo wfptCreateInfo{};
        wfptCreateInfo.colorTarget = m_colorTarget;
        wfptCreateInfo.tlas = m_tlas;
        m_wfptPass = std::unique_ptr<WfptPass>(new WfptPass(wfptCreateInfo));

        m_dirtyPasses = false;
    }
}

void Renderer::Initialize()
{
    m_blasDataPool = std::unique_ptr<BlasDataPool>(new BlasDataPool());

    RecreateRenderTargets();
}

void Renderer::Shutdown()
{
    IdPass::ClearPipelineCache();
    PresentPass::ClearPipelineCache();
    SrgbToLinearPass::ClearPipelineCache();
    WfptPass::ClearPipelineCache();
    WriteIndirectArgsPass::ClearPipelineCache();
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
    RebuildPasses();

    if (m_wfptPass)
    {
        m_wfptPass->seed = frameIdx;
        m_wfptPass->maxBounces = 3;
        m_wfptPass->Dispatch(m_cameras.back(), *m_blasDataPool);
    }

    PresentPass presentPass(m_colorTarget);
    presentPass.Dispatch();

    frameIdx++;
}

TextureHandle Renderer::GetEditorCameraColorTarget()
{
#ifdef BX_EDITOR_BUILD
    return m_colorTarget;// Graphics::GetSwapchainColorTarget();
#else
    return TextureHandle::null;
#endif
}