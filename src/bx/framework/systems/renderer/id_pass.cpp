#include "bx/framework/systems/renderer/id_pass.hpp"

#include "bx/engine/containers/lazy_init.hpp"

#include "bx/framework/components/transform.hpp"
#include "bx/framework/components/mesh_filter.hpp"
#include "bx/framework/components/mesh_renderer.hpp"

const char* ID_PASS_SHADER_SRC = R""""(

#ifdef VERTEX

layout (location = 0) in vec3 Position;

flat out uvec2 Frag_EntityID;

layout (binding = 0, std140) uniform ConstantBuffer
{
    mat4 ViewProjMtx;
};

layout (binding = 1, std140) uniform ModelBuffer
{
    mat4 WorldMeshMtx;
    uvec2 EntityID;
};

void main()
{
   gl_Position = ViewProjMtx * WorldMeshMtx * vec4(Position, 1.0);
   Frag_EntityID = EntityID;
}

#endif // VERTEX

#ifdef FRAGMENT

flat in uvec2 Frag_EntityID;

layout (location = 0) out uvec2 Out_Color;

void main()
{
    Out_Color = Frag_EntityID;
}

#endif // FRAGMENT

)"""";

struct VertexMeshUniform
{
    Mat4 world = Mat4::Identity();
    u64 entityId;
};

struct IdPipelineArgs
{
    TextureHandle colorTarget;
    TextureHandle depthTarget;

    b8 operator==(const IdPipelineArgs& other) const { return colorTarget == other.colorTarget && depthTarget == other.depthTarget; }
};

template <>
struct std::hash<IdPipelineArgs>
{
    std::size_t operator()(const IdPipelineArgs& h) const
    {
        // TODO: fix
        return std::hash<TextureHandle>()(h.colorTarget) & std::hash<TextureHandle>()(h.depthTarget);
    }
};

struct IdPipeline : public LazyInitMap<IdPipeline, GraphicsPipelineHandle, IdPipelineArgs>
{
    IdPipeline(const IdPipelineArgs& args)
    {
        ShaderCreateInfo vertexCreateInfo{};
        vertexCreateInfo.name = "Id Vertex Shader";
        vertexCreateInfo.shaderType = ShaderType::VERTEX;
        vertexCreateInfo.src = ID_PASS_SHADER_SRC;
        ShaderHandle vertexShader = Graphics::CreateShader(vertexCreateInfo);

        ShaderCreateInfo fragmentCreateInfo{};
        fragmentCreateInfo.name = "Id Fragment Shader";
        fragmentCreateInfo.shaderType = ShaderType::FRAGMENT;
        fragmentCreateInfo.src = ID_PASS_SHADER_SRC;
        ShaderHandle fragmentShader = Graphics::CreateShader(fragmentCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::VERTEX, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::VERTEX, BindingTypeDescriptor::UniformBuffer())
            })
        };

        VertexBufferLayout vertexBufferLayout{};
        vertexBufferLayout.stride = sizeof(Mesh::Vertex);
        vertexBufferLayout.attributes = {
            VertexAttribute(VertexFormat::FLOAT_32X3, 0, 0)
        };

        ColorTargetState colorTargetState{};
        colorTargetState.format = Graphics::GetTextureCreateInfo(args.colorTarget).format;

        TextureFormat depthFormat = Graphics::GetTextureCreateInfo(args.depthTarget).format;

        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Id Pipeline";
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

HashMap<IdPipelineArgs, std::unique_ptr<IdPipeline>> LazyInitMap<IdPipeline, GraphicsPipelineHandle, IdPipelineArgs>::cache = {};

IdPass::IdPass(TextureHandle colorTarget, TextureHandle depthTarget)
    : colorTarget(colorTarget), depthTarget(depthTarget)
{
    const TextureCreateInfo& textureCreateInfo = Graphics::GetTextureCreateInfo(colorTarget);
    width = textureCreateInfo.size.width;
    height = textureCreateInfo.size.height;

    colorTargetView = Graphics::CreateTextureView(colorTarget);
    depthTargetView = Graphics::CreateTextureView(depthTarget);

    BufferCreateInfo constantBufferCreateInfo{};
    constantBufferCreateInfo.name = "Id Pass Constant Buffer";
    constantBufferCreateInfo.size = sizeof(Mat4);
    constantBufferCreateInfo.usageFlags = BufferUsageFlags::COPY_DST | BufferUsageFlags::UNIFORM;
    constantBuffer = Graphics::CreateBuffer(constantBufferCreateInfo);
}

IdPass::~IdPass()
{
    Graphics::DestroyBuffer(constantBuffer);
    Graphics::DestroyTextureView(colorTargetView);
    Graphics::DestroyTextureView(depthTargetView);
}

void IdPass::Dispatch(const Camera& camera)
{
    Graphics::WriteBuffer(constantBuffer, 0, &camera.GetViewProjection(), sizeof(Mat4));

    RenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.name = "Id";
    renderPassDescriptor.colorAttachments = { RenderPassColorAttachment(colorTargetView) };
    renderPassDescriptor.depthStencilAttachment = Optional<RenderPassDepthStencilAttachment>::Some(RenderPassDepthStencilAttachment(depthTargetView));

    RenderPassHandle renderPass = Graphics::BeginRenderPass(renderPassDescriptor);
    {
        GraphicsPipelineHandle pipeline = IdPipeline::Get({ colorTarget, depthTarget });
        Graphics::SetGraphicsPipeline(pipeline);

        EntityManager::ForEach<Transform, MeshFilter, MeshRenderer>(
            [&](Entity entity, const Transform& trx, const MeshFilter& mf, const MeshRenderer& mr)
            {
                for (const auto& mesh : mf.GetMeshes())
                {
                    if (!mesh) continue;
                    const auto& meshData = mesh.GetData();

                    VertexMeshUniform meshUniform{};
                    meshUniform.world = trx.GetMatrix() * meshData.GetMatrix();
                    meshUniform.entityId = entity.GetId();

                    BufferCreateInfo meshUniformCreateInfo{};
                    meshUniformCreateInfo.name = "Mesh Uniform";
                    meshUniformCreateInfo.size = sizeof(VertexMeshUniform);
                    meshUniformCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
                    BufferHandle meshUniformBuffer = Graphics::CreateBuffer(meshUniformCreateInfo, &meshUniform);
                
                    BindGroupCreateInfo createInfo{};
                    createInfo.name = "Id Pass BindGroup";
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

void IdPass::ClearPipelineCache()
{
    IdPipeline::Clear();
}