#include "bx/framework/systems/renderer/gbuffer_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/framework/components/transform.hpp"
#include "bx/framework/components/mesh_filter.hpp"
#include "bx/framework/components/mesh_renderer.hpp"

struct GBufferConstants
{
    Mat4 viewProj;
    Vec3 viewPos;
    u32 _PADDING0;
};

struct VertexMeshUniform
{
    Mat4 world = Mat4::Identity();
    u32 blasInstanceIdx;
    u32 _PADDING0;
};

struct GBufferPipelineArgs
{
    TextureHandle colorTarget;
    TextureHandle depthTarget;

    b8 operator==(const GBufferPipelineArgs& other) const { return colorTarget == other.colorTarget && depthTarget == other.depthTarget; }
};

template <>
struct std::hash<GBufferPipelineArgs>
{
    std::size_t operator()(const GBufferPipelineArgs& h) const
    {
        // TODO: fix
        return std::hash<TextureHandle>()(h.colorTarget) & std::hash<TextureHandle>()(h.depthTarget);
    }
};

struct GBufferPipeline : public LazyInitMap<GBufferPipeline, GraphicsPipelineHandle, GBufferPipelineArgs>
{
    GBufferPipeline(const GBufferPipelineArgs& args)
    {
        ShaderCreateInfo vertexCreateInfo{};
        vertexCreateInfo.name = "GBuffer Vertex Shader";
        vertexCreateInfo.shaderType = ShaderType::VERTEX;
        vertexCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/gbuffer/gbuffer.vert.shader")));
        ShaderHandle vertexShader = Graphics::CreateShader(vertexCreateInfo);

        ShaderCreateInfo fragmentCreateInfo{};
        fragmentCreateInfo.name = "GBuffer Fragment Shader";
        fragmentCreateInfo.shaderType = ShaderType::FRAGMENT;
        fragmentCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/gbuffer/gbuffer.frag.shader")));
        ShaderHandle fragmentShader = Graphics::CreateShader(fragmentCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::VERTEX, BindingTypeDescriptor::UniformBuffer())
            })
        };

        VertexBufferLayout vertexBufferLayout{};
        vertexBufferLayout.stride = sizeof(Mesh::Vertex);
        vertexBufferLayout.attributes = {
            VertexAttribute(VertexFormat::FLOAT_32X3, offsetof(Mesh::Vertex, position), 0),
            VertexAttribute(VertexFormat::FLOAT_32X4, offsetof(Mesh::Vertex, color), 1),
            VertexAttribute(VertexFormat::SINT_32X4,  offsetof(Mesh::Vertex, bones), 2),
            VertexAttribute(VertexFormat::FLOAT_32X4, offsetof(Mesh::Vertex, weights), 3),
            VertexAttribute(VertexFormat::FLOAT_32X3, offsetof(Mesh::Vertex, normal), 4),
            VertexAttribute(VertexFormat::FLOAT_32X3, offsetof(Mesh::Vertex, tangent), 5),
            VertexAttribute(VertexFormat::FLOAT_32X2, offsetof(Mesh::Vertex, uv), 6)
        };

        ColorTargetState colorTargetState{};
        colorTargetState.format = Graphics::GetTextureCreateInfo(args.colorTarget).format;

        TextureFormat depthFormat = Graphics::GetTextureCreateInfo(args.depthTarget).format;

        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "GBuffer Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.vertexShader = vertexShader;
        pipelineCreateInfo.fragmentShader = fragmentShader;
        pipelineCreateInfo.vertexBuffers = { vertexBufferLayout };
        pipelineCreateInfo.cullMode = Optional<Face>::None();
        pipelineCreateInfo.colorTarget = Optional<ColorTargetState>::Some(colorTargetState);
        pipelineCreateInfo.depthFormat = Optional<TextureFormat>::Some(depthFormat);
        data = Graphics::CreateGraphicsPipeline(pipelineCreateInfo);

        Graphics::DestroyShader(vertexShader);
        Graphics::DestroyShader(fragmentShader);
    }
};

template<>
HashMap<GBufferPipelineArgs, std::unique_ptr<GBufferPipeline>> LazyInitMap<GBufferPipeline, GraphicsPipelineHandle, GBufferPipelineArgs>::cache = {};

GBufferPass::GBufferPass(TextureHandle depthTarget)
    : depthTarget(depthTarget)
{
    const TextureCreateInfo& textureCreateInfo = Graphics::GetTextureCreateInfo(depthTarget);
    width = textureCreateInfo.size.width;
    height = textureCreateInfo.size.height;

    TextureCreateInfo colorTargetCreateInfo{};
    colorTargetCreateInfo.name = "GBuffer Color Target";
    colorTargetCreateInfo.size = Extend3D(width, height, 1);
    colorTargetCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    colorTargetCreateInfo.usageFlags = TextureUsageFlags::RENDER_ATTACHMENT | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::STORAGE_BINDING;
    colorTarget = Graphics::CreateTexture(colorTargetCreateInfo);

    colorTargetView = Graphics::CreateTextureView(colorTarget);
    depthTargetView = Graphics::CreateTextureView(depthTarget);

    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "GBuffer Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(GBufferConstants);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);
}

GBufferPass::~GBufferPass()
{
    Graphics::DestroyTexture(colorTarget);
    Graphics::DestroyBuffer(constantBuffer);
    Graphics::DestroyTextureView(colorTargetView);
    Graphics::DestroyTextureView(depthTargetView);
}

TextureViewHandle GBufferPass::GetColorTargetView() const
{
    return colorTargetView;
}

void GBufferPass::Dispatch(const Camera& camera)
{
    GBufferConstants constants{};
    constants.viewProj = camera.GetViewProjection();
    Mat4::Decompose(camera.GetView().Inverse(), constants.viewPos, Quat{}, Vec3{});
    Graphics::WriteBuffer(constantBuffer, 0, &constants, sizeof(GBufferConstants));

    RenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.name = "GBuffer";
    renderPassDescriptor.colorAttachments = { RenderPassColorAttachment(colorTargetView) };
    renderPassDescriptor.depthStencilAttachment = Optional<RenderPassDepthStencilAttachment>::Some(RenderPassDepthStencilAttachment(depthTargetView));

    RenderPassHandle renderPass = Graphics::BeginRenderPass(renderPassDescriptor);
    {
        GraphicsPipelineHandle pipeline = GBufferPipeline::Get({ colorTarget, depthTarget });
        Graphics::SetGraphicsPipeline(pipeline);

        EntityManager::ForEach<Transform, MeshFilter, MeshRenderer>(
            [&](Entity entity, const Transform& trx, const MeshFilter& mf, const MeshRenderer& mr)
            {
                for (u32 i = 0; i < mf.GetMeshes().size(); i++)
                {
                    const auto& mesh = mf.GetMesh(i);
                    if (!mesh) continue;
                    const auto& meshData = mesh.GetData();

                    VertexMeshUniform meshUniform{};
                    meshUniform.world = trx.GetMatrix() * meshData.GetMatrix();
                    meshUniform.blasInstanceIdx = mf.m_blasInstanceIndices[i];

                    BufferCreateInfo meshUniformCreateInfo{};
                    meshUniformCreateInfo.name = "Mesh Uniform";
                    meshUniformCreateInfo.size = sizeof(VertexMeshUniform);
                    meshUniformCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
                    meshUniformCreateInfo.data = &meshUniform;
                    BufferHandle meshUniformBuffer = Graphics::CreateBuffer(meshUniformCreateInfo);

                    BindGroupCreateInfo createInfo{};
                    createInfo.name = "GBuffer Pass BindGroup";
                    createInfo.layout = Graphics::GetBindGroupLayout(pipeline, 0);
                    createInfo.entries = {
                        BindGroupEntry(0, BindingResource::Buffer(BufferBinding(constantBuffer))),
                        BindGroupEntry(1, BindingResource::Buffer(BufferBinding(meshUniformBuffer)))
                    };
                    BindGroupHandle bindGroup = Graphics::CreateBindGroup(createInfo);

                    Graphics::SetVertexBuffer(0, BufferSlice(meshData.GetVertexBuffer()));
                    Graphics::SetIndexBuffer(BufferSlice(meshData.GetIndexBuffer()), IndexFormat::UINT32);
                    Graphics::SetBindGroup(0, bindGroup);
                    Graphics::DrawIndexed(meshData.GetIndices().size());

                    Graphics::DestroyBindGroup(bindGroup);
                    Graphics::DestroyBuffer(meshUniformBuffer);
                }
            });
    }
    Graphics::EndRenderPass(renderPass);
}

void GBufferPass::ClearPipelineCache()
{
    GBufferPipeline::Clear();
}