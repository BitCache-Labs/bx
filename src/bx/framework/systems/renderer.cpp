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

#include <bx/engine/core/file.hpp>
#include <bx/engine/core/data.hpp>
#include <bx/engine/core/profiler.hpp>
#include <bx/engine/core/resource.hpp>
#include <bx/engine/containers/tree.hpp>
#include <bx/engine/modules/graphics.hpp>
#include <bx/engine/modules/window.hpp>

struct VertexConstantsUniform
{
    Mat4 view = Mat4::Identity();
    Mat4 projection = Mat4::Identity();
    Mat4 viewProjection = Mat4::Identity();
};

struct VertexMeshUniform
{
    Mat4 model = Mat4::Identity();
    Mat4 boneToMesh = Mat4::Identity();
    Vec4i lightIndices = Vec4i(-1, -1, -1, -1);
};

struct LightSourceData
{
    u32 type = 0;
    f32 intensity = 1.0f;

    i32 shadowMapIndex = -1;
    u32 cascadeCount = 0;

    Vec3 position = Vec3(0, 0, 0);
    f32 constant = 1.0f;

    Vec3 direction = Vec3(0, 0, 0);
    f32 linear_cutoff = 0.1f;

    Vec3 color = Vec3(1, 1, 1);
    f32 quadratic_outerCutoff = 0.01f;
};

struct RendererState : NoCopy
{
    HashMap<ResourceHandle, GraphicsPipelineHandle> shaderPipelines{};

    TextureHandle colorTarget = TextureHandle::null;
    TextureHandle depthTarget = TextureHandle::null;

    BufferHandle vertexConstantsBuffer = BufferHandle::null;
    BufferHandle lightSourceBuffer = BufferHandle::null;
};
static std::unique_ptr<RendererState> s = nullptr;

void BuildShaderPipelines()
{
    VertexBufferLayout vertexBufferLayout{};
    vertexBufferLayout.stride = sizeof(Mesh::Vertex);
    vertexBufferLayout.attributes = {
        VertexAttribute(VertexFormat::FLOAT_32X3, offsetof(Mesh::Vertex, position), 0),
        VertexAttribute(VertexFormat::FLOAT_32X4, offsetof(Mesh::Vertex, color),    1),
        VertexAttribute(VertexFormat::FLOAT_32X3, offsetof(Mesh::Vertex, normal),   2),
        VertexAttribute(VertexFormat::FLOAT_32X3, offsetof(Mesh::Vertex, tangent),  3),
        VertexAttribute(VertexFormat::FLOAT_32X2, offsetof(Mesh::Vertex, uv),       4),
        VertexAttribute(VertexFormat::SINT_32X4,  offsetof(Mesh::Vertex, bones),    5),
        VertexAttribute(VertexFormat::FLOAT_32X4, offsetof(Mesh::Vertex, weights),  6)
    };

    PipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayouts = {
        BindGroupLayoutDescriptor(0, {
            BindGroupLayoutEntry(0, ShaderStageFlags::VERTEX, BindingTypeDescriptor::UniformBuffer()),      // layout (binding = 0, std140) uniform Constants
            BindGroupLayoutEntry(1, ShaderStageFlags::VERTEX, BindingTypeDescriptor::UniformBuffer()),      // layout (binding = 1, std140) uniform Model
            BindGroupLayoutEntry(2, ShaderStageFlags::VERTEX, BindingTypeDescriptor::UniformBuffer()),      // layout (binding = 2, std140) uniform Animation
            BindGroupLayoutEntry(4, ShaderStageFlags::FRAGMENT, BindingTypeDescriptor::UniformBuffer())     // layout (binding = 4, std140) uniform LightBuffer
        }),
        Material::GetBindGroupLayout()
    };

    ColorTargetState colorTargetState{};
    colorTargetState.format = Graphics::GetTextureCreateInfo(Graphics::GetSwapchainColorTarget()).format;

    TextureFormat depthFormat = Graphics::GetTextureCreateInfo(s->depthTarget).format;

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

                const Material& materialData = material.GetData();
                const Resource<Shader>& shaderResource = materialData.GetShader();
                const Shader& shader = shaderResource.GetData();

                if (s->shaderPipelines.find(shaderResource.GetHandle()) == s->shaderPipelines.end())
                {
                    GraphicsPipelineCreateInfo createInfo{};
                    createInfo.name = "Shader Pipeline";
                    createInfo.vertexShader = shader.GetVertexShader();
                    createInfo.fragmentShader = shader.GetFragmentShader();
                    createInfo.vertexBuffers = { vertexBufferLayout };
                    createInfo.colorTarget = Optional<ColorTargetState>::Some(colorTargetState);
                    createInfo.cullMode = Optional<Face>::Some(Face::BACK);
                    createInfo.layout = pipelineLayoutDescriptor;
                    createInfo.depthFormat = Optional<TextureFormat>::Some(depthFormat);

                    GraphicsPipelineHandle graphicsPipeline = Graphics::CreateGraphicsPipeline(createInfo);
                    s->shaderPipelines.insert(std::make_pair(shaderResource.GetHandle(), graphicsPipeline));
                }
            }
        });
}

void UpdateAnimators()
{
    EntityManager::ForEach<Animator>(
        [&](Entity entity, Animator& anim)
        {
            anim.Update();
        });
}

void UpdateLightSources()
{
    List<LightSourceData> lightSources{};

    EntityManager::ForEach<Transform, Light>(
        [&](Entity entity, const Transform& trx, const Light& l)
        {
            LightSourceData lightSource;
            lightSource.position = trx.GetPosition();
            lightSource.intensity = l.GetIntensity();
            lightSource.constant = l.GetConstant();
            lightSource.linear_cutoff = l.GetLinear();
            lightSource.quadratic_outerCutoff = l.GetQuadratic();
            lightSource.color = l.GetColor();
            lightSources.emplace_back(lightSource);
        });

    Graphics::WriteBuffer(s->lightSourceBuffer, 0, lightSources.data(), lightSources.size() * sizeof(LightSourceData));
}

void Renderer::UpdateCameras()
{
    List<VertexConstantsUniform> viewConstants{};

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

            VertexConstantsUniform constants;
            constants.view = camera.GetView();
            constants.projection = camera.GetProjection();
            constants.viewProjection = camera.GetViewProjection();
            viewConstants.emplace_back(constants);
        });

    if (editorCamera.IsSome())
    {
        Camera& camera = editorCamera.Unwrap();

        VertexConstantsUniform constants;
        constants.view = camera.GetView();
        constants.projection = camera.GetProjection();
        constants.viewProjection = camera.GetViewProjection();
        viewConstants.emplace_back(constants);
    }

    // TODO: for now just uploading last camera, maybe splitscreen support?
    if (viewConstants.size() > 0)
    {
        Graphics::WriteBuffer(s->vertexConstantsBuffer, 0, &viewConstants.back(), Math::Min(viewConstants.size(), static_cast<SizeType>(1)) * sizeof(VertexConstantsUniform));
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
        colorTargetCreateInfo.usageFlags = TextureUsageFlags::RENDER_ATTACHMENT;
        if (s->colorTarget) Graphics::DestroyTexture(s->colorTarget);
        s->colorTarget = Graphics::CreateTexture(colorTargetCreateInfo);

        TextureCreateInfo depthTargetCreateInfo{};
        depthTargetCreateInfo.name = "Depth Target";
        depthTargetCreateInfo.size = Extend3D(w, h, 1);
        depthTargetCreateInfo.format = TextureFormat::DEPTH24_PLUS_STENCIL8;
        depthTargetCreateInfo.usageFlags = TextureUsageFlags::RENDER_ATTACHMENT;
        if (s->depthTarget) Graphics::DestroyTexture(s->depthTarget);
        s->depthTarget = Graphics::CreateTexture(depthTargetCreateInfo);

        // TODO: temporary safety, this line is unnecessary as long as the color target format doesn't change (except for the first time)
        s->shaderPipelines.clear();
    }
}

void Renderer::Initialize()
{
    s = std::make_unique<RendererState>();

    BufferCreateInfo vertexConstantsCreateInfo{};
    vertexConstantsCreateInfo.name = "Vertex Constants";
    vertexConstantsCreateInfo.size = sizeof(VertexConstantsUniform);
    vertexConstantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM | BufferUsageFlags::COPY_DST;
    s->vertexConstantsBuffer = Graphics::CreateBuffer(vertexConstantsCreateInfo);

    BufferCreateInfo lightSourceCreateInfo{};
    lightSourceCreateInfo.name = "Light Sources";
    lightSourceCreateInfo.size = sizeof(LightSourceData) * 10;
    lightSourceCreateInfo.usageFlags = BufferUsageFlags::UNIFORM | BufferUsageFlags::COPY_DST;
    s->lightSourceBuffer = Graphics::CreateBuffer(lightSourceCreateInfo);
    
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
    UpdateLightSources();
    UpdateCameras();
    BuildShaderPipelines();

    Graphics::UpdateDebugLines();

    TextureViewHandle colorTargetView = Graphics::CreateTextureView(s->colorTarget);
    TextureViewHandle depthTargetView = Graphics::CreateTextureView(s->depthTarget);

    RenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.name = "Draw Meshes Pass";
    renderPassDescriptor.colorAttachments = { RenderPassColorAttachment(colorTargetView) };
    renderPassDescriptor.depthStencilAttachment = Optional<RenderPassDepthStencilAttachment>::Some(depthTargetView);

    RenderPassHandle renderPass = Graphics::BeginRenderPass(renderPassDescriptor);
    {
        EntityManager::ForEach<Transform, MeshFilter, MeshRenderer>(
            [&](Entity entity, const Transform& trx, const MeshFilter& mf, const MeshRenderer& mr)
            {
                if (mr.GetMaterialCount() == 0)
                    return;

                BufferHandle animatorBonesBuffer;
                if (entity.HasComponent<Animator>())
                {
                    const auto& anim = entity.GetComponent<Animator>();
                    animatorBonesBuffer = anim.GetBoneBuffer();
                }
                else
                {
                    animatorBonesBuffer = Graphics::EmptyBuffer();
                }

                SizeType index = 0;
                for (const auto& mesh : mf.GetMeshes())
                {
                    const auto& material = mr.GetMaterial(index++);
                    index %= mr.GetMaterialCount();

                    if (!mesh || !material)
                        continue;

                    const auto& meshData = mesh.GetData();
                    const auto& materialData = material.GetData();
                    const auto& shader = materialData.GetShader();

                    auto& graphicsPipelineIter = s->shaderPipelines.find(shader.GetHandle());
                    BX_ASSERT(graphicsPipelineIter != s->shaderPipelines.end(), "Missing graphics pipeline, this should not happen.");
                    GraphicsPipelineHandle graphicsPipeline = graphicsPipelineIter->second;

                    VertexMeshUniform meshUniform{};
                    meshUniform.boneToMesh = meshData.GetMatrix();
                    meshUniform.model = trx.GetMatrix();
                    meshUniform.lightIndices = Vec4i(0, 1, 2, 3);

                    // TODO: should be push constants (need to be emulated on opengl)
                    // TODO: use same patterns as the animator bones buffer
                    BufferCreateInfo meshUniformCreateInfo{};
                    meshUniformCreateInfo.name = "Mesh Uniform Buffer";
                    meshUniformCreateInfo.size = sizeof(VertexMeshUniform);
                    meshUniformCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
                    BufferHandle meshUniformBuffer = Graphics::CreateBuffer(meshUniformCreateInfo, &meshUniform);

                    // TODO: very lazy, shouldn't be created every frame probably
                    BindGroupCreateInfo createInfo{};
                    createInfo.name = "Renderer Core BindGroup";
                    createInfo.layout = Graphics::GetBindGroupLayout(graphicsPipeline, 0);
                    createInfo.entries = {
                        BindGroupEntry(0, BindingResource::Buffer(BufferBinding(s->vertexConstantsBuffer))),
                        BindGroupEntry(1, BindingResource::Buffer(BufferBinding(meshUniformBuffer))),
                        BindGroupEntry(2, BindingResource::Buffer(BufferBinding(animatorBonesBuffer))),
                        BindGroupEntry(4, BindingResource::Buffer(BufferBinding(s->lightSourceBuffer)))
                    };
                    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

                    BindGroupLayoutHandle bindGroupLayout = Graphics::GetBindGroupLayout(graphicsPipeline, Material::SHADER_BIND_GROUP);
                    BindGroupHandle materialBindGroup = materialData.GetBindGroup(bindGroupLayout);

                    Graphics::SetGraphicsPipeline(graphicsPipeline);
                    Graphics::SetVertexBuffer(0, BufferSlice(meshData.GetVertexBuffer()));
                    Graphics::SetIndexBuffer(BufferSlice(meshData.GetIndexBuffer()), IndexFormat::UINT32);
                    Graphics::SetBindGroup(0, bindGroup);
                    Graphics::SetBindGroup(Material::SHADER_BIND_GROUP, materialBindGroup);
                    Graphics::DrawIndexed(meshData.GetIndices().size());

                    Graphics::DestroyBindGroup(bindGroup);
                    Graphics::DestroyBuffer(meshUniformBuffer);
                }
            });
    }
    Graphics::EndRenderPass(renderPass);

    PresentPass presentPass(s->colorTarget);
    presentPass.Dispatch();
}

TextureHandle Renderer::GetEditorCameraColorTarget()
{
#ifdef BX_EDITOR_BUILD
    return Graphics::GetSwapchainColorTarget();
#else
    return TextureHandle::null;
#endif
}