#include "bx/framework/systems/renderer/restir_di_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"
#include "bx/framework/resources/shader.hpp"

#include "bx/engine/core/file.hpp"

struct SpatialReusePipeline : public LazyInit<SpatialReusePipeline, ComputePipelineHandle>
{
    SpatialReusePipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Restir Spatial Reuse Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/restir/spatial_reuse.shader")));;
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageTexture(StorageTextureAccess::WRITE, TextureFormat::RGBA32_FLOAT))
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Restir Spatial Reuse Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};
template<>
std::unique_ptr<v> LazyInit<SpatialReusePipeline, ComputePipelineHandle>::cache = nullptr;

RestirDiPass::RestirDiPass(BufferHandle sampleBuffer)
{

}

RestirDiPass::~RestirDiPass()
{
	Graphics::DestroyBindGroup(bindGroup);
}

void RestirDiPass::Dispatch()
{

}

void RestirDiPass::ClearPipelineCache()
{

}