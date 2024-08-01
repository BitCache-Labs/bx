#include "bx/framework/systems/renderer/present_pass.hpp"

#include "bx/engine/containers/lazy_init.hpp"

const char* PRESENT_SHADER_SRC = R""""(

#ifdef VERTEX

layout (location = 0) out vec2 fragTexCoord;

void main()
{
	vec2 vertices[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));
	gl_Position = vec4(vertices[gl_VertexID],0,1);
	fragTexCoord = 0.5 * gl_Position.xy + vec2(0.5);
}

#endif // VERTEX

#ifdef FRAGMENT

layout (location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D colorImage;

vec3 aces(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 gammaCorrect(vec3 x, float gamma)
{
    return pow(x, vec3(1.0 / gamma));
}

void main()
{
    vec3 hdrColor = texture(colorImage, fragTexCoord).rgb;

    // acesg -> acescct (matrix mult)

    // 2 luts, 1d & 3d textures
    // acescct , apply output transform (monitor dependant)
    
    
    vec3 sdrColor = aces(hdrColor);
    sdrColor = gammaCorrect(sdrColor, 2.2);

    outColor = vec4(sdrColor, 1.0);
}

#endif // FRAGMENT

)"""";

struct PresentPipeline : public LazyInit<PresentPipeline, GraphicsPipelineHandle>
{
    PresentPipeline()
    {
        ShaderCreateInfo vertexCreateInfo{};
        vertexCreateInfo.name = "Present Vertex Shader";
        vertexCreateInfo.shaderType = ShaderType::VERTEX;
        vertexCreateInfo.src = PRESENT_SHADER_SRC;
        ShaderHandle vertexShader = Graphics::CreateShader(vertexCreateInfo);

        ShaderCreateInfo fragmentCreateInfo{};
        fragmentCreateInfo.name = "Present Fragment Shader";
        fragmentCreateInfo.shaderType = ShaderType::FRAGMENT;
        fragmentCreateInfo.src = PRESENT_SHADER_SRC;
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
        pipelineCreateInfo.colorTarget = Optional<ColorTargetState>::Some(colorTargetState);
        data = Graphics::CreateGraphicsPipeline(pipelineCreateInfo);

        Graphics::DestroyShader(vertexShader);
        Graphics::DestroyShader(fragmentShader);
    }
};

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