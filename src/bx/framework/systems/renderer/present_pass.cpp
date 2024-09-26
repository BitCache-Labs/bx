#include "bx/framework/systems/renderer/present_pass.hpp"

#include "bx/engine/core/file.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

struct PresentPipeline : public LazyInit<PresentPipeline, GraphicsPipelineHandle>
{
    PresentPipeline()
    {
        ShaderCreateInfo vertexCreateInfo{};
        vertexCreateInfo.name = "Present Vertex Shader";
        vertexCreateInfo.shaderType = ShaderType::VERTEX;
        vertexCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/present/present.vert.shader")));
        ShaderHandle vertexShader = Graphics::CreateShader(vertexCreateInfo);

        ShaderCreateInfo fragmentCreateInfo{};
        fragmentCreateInfo.name = "Present Fragment Shader";
        fragmentCreateInfo.shaderType = ShaderType::FRAGMENT;
        fragmentCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/passes/present/present.frag.shader")));
        ShaderHandle fragmentShader = Graphics::CreateShader(fragmentCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::FRAGMENT, BindingTypeDescriptor::Texture(TextureSampleType::FLOAT)),
            })
        };

        ColorTargetState colorTargetState{};
        colorTargetState.format = Graphics::GetTextureCreateInfo(Graphics::GetSwapchainColorTarget()).format;

        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Present Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.vertexShader = vertexShader;
        pipelineCreateInfo.fragmentShader = fragmentShader;
        pipelineCreateInfo.vertexBuffers = {};
        pipelineCreateInfo.cullMode = Optional<Face>::None();
        pipelineCreateInfo.colorTargets = List<ColorTargetState>{ colorTargetState };
        data = Graphics::CreateGraphicsPipeline(pipelineCreateInfo);

        Graphics::DestroyShader(vertexShader);
        Graphics::DestroyShader(fragmentShader);
    }
};

template<>
std::unique_ptr<PresentPipeline> LazyInit<PresentPipeline, GraphicsPipelineHandle>::cache = nullptr;

PresentPass::PresentPass(TextureHandle texture)
{
    const TextureCreateInfo& textureCreateInfo = Graphics::GetTextureCreateInfo(texture);

    width = textureCreateInfo.size.width;
    height = textureCreateInfo.size.height;

    textureView = Graphics::CreateTextureView(texture);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Present BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(PresentPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::TextureView(textureView)),
    };
    bindGroup = Graphics::CreateBindGroup(createInfo);
}

PresentPass::~PresentPass()
{
    Graphics::DestroyBindGroup(bindGroup);
    Graphics::DestroyTextureView(textureView);
}

void PresentPass::Dispatch()
{
    RenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.name = "Present";
    renderPassDescriptor.colorAttachments = { RenderPassColorAttachment(Graphics::GetSwapchainColorTargetView()) };

    RenderPassHandle renderPass = Graphics::BeginRenderPass(renderPassDescriptor);
    {
        Graphics::SetGraphicsPipeline(PresentPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::Draw(3);
    }
    Graphics::EndRenderPass(renderPass);
}

void PresentPass::ClearPipelineCache()
{
    PresentPipeline::Clear();
}