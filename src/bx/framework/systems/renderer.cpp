#include "bx/framework/systems/renderer.hpp"

#include "bx/framework/components/transform.hpp"
#include "bx/framework/components/mesh_filter.hpp"
#include "bx/framework/components/mesh_renderer.hpp"
#include "bx/framework/components/animator.hpp"
#include "bx/framework/components/light.hpp"

#include "bx/framework/systems/renderer/gbuffer_pass.hpp"
#include "bx/framework/systems/renderer/id_pass.hpp"
#include "bx/framework/systems/renderer/present_pass.hpp"
#include "bx/framework/systems/renderer/restir_di_pass.hpp"
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
        [&](Entity entity, const Transform& trx, MeshFilter& mf, const MeshRenderer& mr)
        {
            mf.m_blasInstanceIndices.clear();

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

                u32 materialIdx = m_materialPool->SubmitInstance(material.GetData(), material.GetHandle());

                u32 blasInstanceIdx;
                if (entity.HasComponent<Animator>())
                {
                    const Animator& animator = entity.GetComponent<Animator>();
                    blasInstanceIdx = m_blasDataPool->SubmitAnimatedInstance(mesh, animator, entity.GetId(), matrix, matrix.Inverse().Transpose(), materialIdx, material.GetData().IsEmissive());
                }
                else
                {
                    blasInstanceIdx = m_blasDataPool->SubmitInstance(mesh, matrix, matrix.Inverse().Transpose(), materialIdx, material.GetData().IsEmissive());
                }

                //BlasInstance blasInstance{};
                //blasInstance.transform = matrix;
                //blasInstance.instanceCustomIndex = blasInstances.size();
                //blasInstance.mask = 0xFF;
                //blasInstance.blas = mesh->GetBlas();
                //blasInstances.push_back(blasInstance);

                mf.m_blasInstanceIndices.push_back(blasInstanceIdx);
            }
        });

    EntityManager::ForEach<Transform, MeshFilter, MeshRenderer>(
        [&](Entity entity, const Transform& trx, MeshFilter& mf, const MeshRenderer& mr)
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

                BlasInstance blasInstance{};
                blasInstance.transform = matrix;
                blasInstance.instanceCustomIndex = blasInstances.size();
                blasInstance.mask = 0xFF;
                blasInstance.blas = mesh->GetBlas();
                blasInstances.push_back(blasInstance);
            }
        });

    m_blasDataPool->Submit();
    m_materialPool->Submit();

    if (!blasInstances.empty()) // TODO: handle empty tlases internally
    {
        TlasCreateInfo tlasCreateInfo{};
        tlasCreateInfo.name = "Dynamic Tlas";
        tlasCreateInfo.blasInstances = blasInstances;
        if (m_tlas) Graphics::DestroyTlas(m_tlas);
        m_tlas = Graphics::CreateTlas(tlasCreateInfo);

        if (m_nertPass)
        {
            m_nertPass->SetTlas(m_tlas);
        }
    }
}

void Renderer::RecreateRenderTargets()
{
    if (Window::WasResized() || !m_colorTarget)
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

        TextureCreateInfo depthTargetCreateInfo{};
        depthTargetCreateInfo.name = "Depth Target";
        depthTargetCreateInfo.size = Extend3D(w, h, 1);
        depthTargetCreateInfo.format = TextureFormat::DEPTH24_PLUS_STENCIL8; // TODO: fix non-stencil format!!
        depthTargetCreateInfo.usageFlags = TextureUsageFlags::RENDER_ATTACHMENT;
        if (m_depthTarget) Graphics::DestroyTexture(m_depthTarget);
        m_depthTarget = Graphics::CreateTexture(depthTargetCreateInfo);

        m_dirtyPasses = true;
    }
}

void Renderer::RebuildPasses()
{
    if (m_dirtyPasses)
    {
        i32 w, h;
        Window::GetSize(&w, &h);

        m_gbufferPass = std::unique_ptr<GBufferPass>(new GBufferPass(m_depthTarget));

        NertCreateInfo nertCreateInfo{};
        nertCreateInfo.colorTarget = m_colorTarget;
        nertCreateInfo.depthTarget = m_depthTarget;
        nertCreateInfo.tlas = m_tlas;
        m_nertPass.reset();
        m_nertPass = std::unique_ptr<NertPass>(new NertPass(nertCreateInfo));

        m_taaPass = std::unique_ptr<TaaPass>(new TaaPass(w, h));

        m_dirtyPasses = false;
    }
}

void Renderer::Initialize()
{
    m_blasDataPool = std::unique_ptr<BlasDataPool>(new BlasDataPool());
    m_materialPool = std::unique_ptr<MaterialPool>(new MaterialPool());
    m_sky = std::unique_ptr<Sky>(new Sky());

    RecreateRenderTargets();
}

void Renderer::Shutdown()
{
    GBufferPass::ClearPipelineCache();
    IdPass::ClearPipelineCache();
    PresentPass::ClearPipelineCache();
    RestirDiPass::ClearPipelineCache();
    NertPass::ClearPipelineCache();
    WriteIndirectArgsPass::ClearPipelineCache();
    TaaPass::ClearPipelineCache();
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

    m_sky->Submit();

    if (m_nertPass) // TODO: remove if statement?
    {
        m_gbufferPass->Dispatch(m_cameras.back());

        m_sky->sunInfo.intensity = 0.0;

        m_nertPass->seed = frameIdx;
        m_nertPass->accumulationFrameIdx = accumulate ? (m_nertPass->accumulationFrameIdx + 1) : 0;
        m_nertPass->maxBounces = 3;
        m_nertPass->unbiased = unbiased;
        m_nertPass->restir = restir;
        m_nertPass->denoise = denoise;
        m_nertPass->antiFirefly = antiFirefly;

        NertDispatchInfo dispatchInfo
        {
            m_cameras.back(),
            *m_blasDataPool,
            *m_materialPool,
            *m_sky,
            m_gbufferPass->GetColorTargetView(),
            m_gbufferPass->GetColorTargetHistoryView(),
            m_gbufferPass->GetVelocityTargetView()
        };
        m_nertPass->Dispatch(dispatchInfo);

        if (taa)
        {
            m_taaPass->Dispatch(m_cameras.back(), m_colorTarget, m_gbufferPass->GetColorTargetView(), m_gbufferPass->GetColorTargetHistoryView(), m_gbufferPass->GetVelocityTargetView());
        }

        m_gbufferPass->NextFrame();
    }

    TextureHandle finalColor = taa ? m_taaPass->GetResolvedColorTarget() : m_colorTarget;

    PresentPass presentPass(finalColor);
    presentPass.Dispatch();

    frameIdx++;
}

TextureHandle Renderer::GetEditorCameraColorTarget()
{
#ifdef BX_EDITOR_BUILD
    return taa ? m_taaPass->GetResolvedColorTarget() : m_colorTarget;
#else
    return TextureHandle::null;
#endif
}