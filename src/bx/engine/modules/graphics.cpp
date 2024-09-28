
#include "bx/engine/modules/graphics.hpp"

#include "bx/engine/modules/graphics/type_validation.hpp"

#include "bx/engine/core/log.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/file.hpp"
#include "bx/engine/core/profiler.hpp"
#include "bx/engine/containers/pool.hpp"

std::unique_ptr<Graphics::CreateInfoCache> Graphics::s_createInfoCache = nullptr;

const GraphicsStats Graphics::GetStats()
{
    GraphicsStats stats{};
    stats.bufferCount = s_createInfoCache->bufferCreateInfos.size();
    stats.samplerCount = s_createInfoCache->samplerCreateInfos.size();
    stats.textureCount = s_createInfoCache->textureCreateInfos.size();
    stats.shaderCount = s_createInfoCache->shaderCreateInfos.size();
    stats.graphicsPipelineCount = s_createInfoCache->graphicsPipelineCreateInfos.size();
    stats.computePipelineCount = s_createInfoCache->computePipelineCreateInfos.size();
    stats.bindGroupCount = s_createInfoCache->bindGroupCreateInfos.size();
    stats.blasCount = s_createInfoCache->blasCreateInfos.size();
    stats.tlasCount = s_createInfoCache->tlasCreateInfos.size();
    return stats;
}

const TextureCreateInfo& Graphics::GetTextureCreateInfo(TextureHandle texture)
{
    BX_ENSURE(texture);

    auto createInfoIter = s_createInfoCache->textureCreateInfos.find(texture);
    BX_ENSURE(createInfoIter != s_createInfoCache->textureCreateInfos.end());
    return createInfoIter->second;
}

const TextureViewCreateInfo& Graphics::GetTextureViewCreateInfo(TextureViewHandle textureView)
{
    BX_ENSURE(textureView);

    auto createInfoIter = s_createInfoCache->textureViewCreateInfos.find(textureView);
    BX_ENSURE(createInfoIter != s_createInfoCache->textureViewCreateInfos.end());
    return createInfoIter->second;
}

const SamplerCreateInfo& Graphics::GetSamplerCreateInfo(SamplerHandle sampler)
{
    BX_ENSURE(sampler);

    auto createInfoIter = s_createInfoCache->samplerCreateInfos.find(sampler);
    BX_ENSURE(createInfoIter != s_createInfoCache->samplerCreateInfos.end());
    return createInfoIter->second;
}

const BufferCreateInfo& Graphics::GetBufferCreateInfo(BufferHandle buffer)
{
    BX_ENSURE(buffer);

    auto createInfoIter = s_createInfoCache->bufferCreateInfos.find(buffer);
    BX_ENSURE(createInfoIter != s_createInfoCache->bufferCreateInfos.end());
    return createInfoIter->second;
}

const ShaderCreateInfo& Graphics::GetShaderCreateInfo(ShaderHandle shader)
{
    BX_ENSURE(shader);

    auto createInfoIter = s_createInfoCache->shaderCreateInfos.find(shader);
    BX_ENSURE(createInfoIter != s_createInfoCache->shaderCreateInfos.end());
    return createInfoIter->second;
}

const GraphicsPipelineCreateInfo& Graphics::GetGraphicsPipelineCreateInfo(GraphicsPipelineHandle graphicsPipeline)
{
    BX_ENSURE(graphicsPipeline);

    auto createInfoIter = s_createInfoCache->graphicsPipelineCreateInfos.find(graphicsPipeline);
    BX_ENSURE(createInfoIter != s_createInfoCache->graphicsPipelineCreateInfos.end());
    return createInfoIter->second;
}

const ComputePipelineCreateInfo& Graphics::GetComputePipelineCreateInfo(ComputePipelineHandle computePipeline)
{
    BX_ENSURE(computePipeline);

    auto createInfoIter = s_createInfoCache->computePipelineCreateInfos.find(computePipeline);
    BX_ENSURE(createInfoIter != s_createInfoCache->computePipelineCreateInfos.end());
    return createInfoIter->second;
}

const BindGroupCreateInfo& Graphics::GetBindGroupCreateInfo(BindGroupHandle bindGroup)
{
    BX_ENSURE(bindGroup);

    auto createInfoIter = s_createInfoCache->bindGroupCreateInfos.find(bindGroup);
    BX_ENSURE(createInfoIter != s_createInfoCache->bindGroupCreateInfos.end());
    return createInfoIter->second;
}

const BlasCreateInfo& Graphics::GetBlasCreateInfo(BlasHandle blas)
{
    BX_ENSURE(blas);

    auto createInfoIter = s_createInfoCache->blasCreateInfos.find(blas);
    BX_ENSURE(createInfoIter != s_createInfoCache->blasCreateInfos.end());
    return createInfoIter->second;
}

const TlasCreateInfo& Graphics::GetTlasCreateInfo(TlasHandle tlas)
{
    BX_ENSURE(tlas);

    auto createInfoIter = s_createInfoCache->tlasCreateInfos.find(tlas);
    BX_ENSURE(createInfoIter != s_createInfoCache->tlasCreateInfos.end());
    return createInfoIter->second;
}

const RenderPassDescriptor& Graphics::GetRenderPassDescriptor(RenderPassHandle renderPass)
{
    BX_ENSURE(renderPass);

    auto createInfoIter = s_createInfoCache->renderPassCreateInfos.find(renderPass);
    BX_ENSURE(createInfoIter != s_createInfoCache->renderPassCreateInfos.end());
    return createInfoIter->second;
}

// TODO: move debug lines somewhere else

struct DebugLineData
{
    DebugLineData() {}
	DebugLineData(const Vec3& a, const Vec3& b, u32 c, f32 l)
		: a(a), b(b), color(c), lifespan(l) {}

    Vec3 a = Vec3(0, 0, 0);
    Vec3 b = Vec3(0, 0, 0);
    u32 color = 0;
    f32 lifespan = 0.0f;
};

static List<DebugLineData> g_debugLines;
static List<DebugLineData> g_debugLinesBuffer;

static List<DebugVertex> g_debugVertices;

void Graphics::DebugLine(const Vec3& a, const Vec3& b, u32 color, f32 lifespan)
{
    g_debugLines.emplace_back(a, b, color, lifespan);
}

void Graphics::UpdateDebugLines()
{
    g_debugVertices.clear();

    g_debugLinesBuffer.clear();
    g_debugLinesBuffer.reserve(g_debugLines.size());
    for (auto& line : g_debugLines)
    {
        g_debugVertices.emplace_back(line.a, line.color);
        g_debugVertices.emplace_back(line.b, line.color);
    
        line.lifespan -= Time::GetDeltaTime();
        if (line.lifespan > 0.0f)
            g_debugLinesBuffer.emplace_back(line);
    }
    g_debugLines = g_debugLinesBuffer;
}

void Graphics::DrawDebugLines(const Mat4& viewProj)
{
    DebugDrawAttribs attribs;
    DebugDraw(viewProj, attribs, g_debugVertices);
}

void Graphics::ClearDebugLines()
{
    g_debugLines.clear();
}