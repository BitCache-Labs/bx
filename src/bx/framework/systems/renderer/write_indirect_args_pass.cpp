#include "bx/framework/systems/renderer/write_indirect_args_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/framework/resources/shader.hpp"

struct Constants
{
    u32 groupSize;
    u32 _PADDING0;
    u32 _PADDING1;
    u32 _PADDING2;
};

struct WriteIndirectArgsPipeline : public LazyInit<WriteIndirectArgsPipeline, ComputePipelineHandle>
{
    WriteIndirectArgsPipeline()
    {
        ShaderCreateInfo shaderCreateInfo{};
        shaderCreateInfo.name = "Write Indirect Args Shader";
        shaderCreateInfo.shaderType = ShaderType::COMPUTE;
        shaderCreateInfo.src = ResolveShaderIncludes(File::ReadTextFile(File::GetPath("[engine]/shaders/utils/write_indirect_args.shader")));
        ShaderHandle shader = Graphics::CreateShader(shaderCreateInfo);

        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayouts = {
            BindGroupLayoutDescriptor(0, {
                BindGroupLayoutEntry(0, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::UniformBuffer()),
                BindGroupLayoutEntry(1, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(true)),
                BindGroupLayoutEntry(2, ShaderStageFlags::COMPUTE, BindingTypeDescriptor::StorageBuffer(false))
            })
        };

        ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.name = "Write Indirect Args Pipeline";
        pipelineCreateInfo.layout = pipelineLayoutDescriptor;
        pipelineCreateInfo.shader = shader;
        data = Graphics::CreateComputePipeline(pipelineCreateInfo);

        Graphics::DestroyShader(shader);
    }
};

template<>
std::unique_ptr<WriteIndirectArgsPipeline> LazyInit<WriteIndirectArgsPipeline, ComputePipelineHandle>::cache = nullptr;

WriteIndirectArgsPass::WriteIndirectArgsPass(BufferHandle countBuffer, u32 groupSize)
{
    BufferCreateInfo indirectArgsCreateInfo{};
    indirectArgsCreateInfo.name = "Indirect Args Buffer";
    indirectArgsCreateInfo.size = 4 * sizeof(u32);
    indirectArgsCreateInfo.usageFlags = BufferUsageFlags::STORAGE | BufferUsageFlags::INDIRECT;
    indirectArgsBuffer = Graphics::CreateBuffer(indirectArgsCreateInfo);

    Constants constantsData{};
    constantsData.groupSize = groupSize;

    BufferCreateInfo constantsCreateInfo{};
    constantsCreateInfo.name = "Write Indirect Args Constants Buffer";
    constantsCreateInfo.size = sizeof(Constants);
    constantsCreateInfo.data = &constantsData;
    constantsCreateInfo.usageFlags = BufferUsageFlags::UNIFORM;
    constantsBuffer = Graphics::CreateBuffer(constantsCreateInfo);

    BindGroupCreateInfo createInfo{};
    createInfo.name = "Write Indirect Args BindGroup";
    createInfo.layout = Graphics::GetBindGroupLayout(WriteIndirectArgsPipeline::Get(), 0);
    createInfo.entries = {
        BindGroupEntry(0, BindingResource::Buffer(constantsBuffer)),
        BindGroupEntry(1, BindingResource::Buffer(countBuffer)),
        BindGroupEntry(2, BindingResource::Buffer(indirectArgsBuffer))
    };
    bindGroup = Graphics::CreateBindGroup(createInfo);
}

WriteIndirectArgsPass::~WriteIndirectArgsPass()
{
    Graphics::DestroyBindGroup(bindGroup);
    Graphics::DestroyBuffer(indirectArgsBuffer);
    Graphics::DestroyBuffer(constantsBuffer);
}

BufferHandle WriteIndirectArgsPass::Dispatch()
{
    ComputePassDescriptor computePassDescriptor{};
    computePassDescriptor.name = "Write Indirect Args";

    ComputePassHandle computePass = Graphics::BeginComputePass(computePassDescriptor);
    {
        Graphics::SetComputePipeline(WriteIndirectArgsPipeline::Get());
        Graphics::SetBindGroup(0, bindGroup);
        Graphics::DispatchWorkgroups(1, 1, 1);
    }
    Graphics::EndComputePass(computePass);

    return indirectArgsBuffer;
}

void WriteIndirectArgsPass::ClearPipelineCache()
{
    WriteIndirectArgsPipeline::Clear();
}