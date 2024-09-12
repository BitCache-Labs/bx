#include "bx/engine/modules/graphics.hpp"

#include "bx/engine/core/log.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/file.hpp"
#include "bx/engine/core/profiler.hpp"
#include "bx/engine/containers/pool.hpp"

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

// TODO: Should the backend be implemented like this?
/*bool Graphics::Initialize(void* device)
{
    auto& backend = GetBackend();
    return backend.Initialize_Impl(device);
}

void Graphics::Shutdown()
{
    auto& backend = GetBackend();
    backend.Shutdown_Impl();
}

void Graphics::Reload()
{
    auto& backend = GetBackend();
    backend.Reload_Impl();
}

void Graphics::SwapBuffers()
{
    auto& backend = GetBackend();
    backend.SwapBuffers_Impl();
}

void Graphics::Render()
{
    auto& backend = GetBackend();
    backend.Render_Impl();
}

TextureFormat Graphics::GetColorBufferFormat()
{
    auto& backend = GetBackend();
    return backend.GetColorBufferFormat_Impl();
}

TextureFormat Graphics::GetDepthBufferFormat()
{
    auto& backend = GetBackend();
    return backend.GetDepthBufferFormat_Impl();
}

GraphicsHandle Graphics::GetCurrentBackBufferRT()
{
    auto& backend = GetBackend();
    return backend.GetCurrentBackBufferRT_Impl();
}

GraphicsHandle Graphics::GetDepthBuffer()
{
    auto& backend = GetBackend();
    return backend.GetDepthBuffer_Impl();
}

GraphicsHandle Graphics::CreateShader(const ShaderInfo& info)
{
    auto& backend = GetBackend();
    return GetBackend().CreateShader_Impl(info);
}

GraphicsHandle Graphics::CreatePipeline(const PipelineInfo& info)
{
    auto& backend = GetBackend();
    return backend.CreatePipeline_Impl(info);
}

void Graphics::ClearRenderTarget(const GraphicsHandle rt, const f32 clearColor[4])
{
    auto& backend = GetBackend();
    backend.ClearRenderTarget_Impl(rt, clearColor);
}

void Graphics::ClearDepthStencil(const GraphicsHandle dt, ClearFlags flags, f32 depth, i32 stencil)
{
    auto& backend = GetBackend();
    backend.ClearDepthStencil_Impl(dt, flags, depth, stencil);
}

GraphicsHandle Graphics::CreateBuffer(const BufferInfo& info)
{
    auto& backend = GetBackend();
    return backend.CreateBuffer_Impl(info);
}

GraphicsHandle Graphics::CreateBuffer(const BufferInfo& info, const BufferData& data)
{
    auto& backend = GetBackend();
    return backend.CreateBuffer_Impl(info, data);
}

void Graphics::UpdateBuffer(const GraphicsHandle buffer, const BufferData& data)
{
    auto& backend = GetBackend();
    backend.UpdateBuffer_Impl(buffer, data);
}

void Graphics::SetStaticVariable(const GraphicsHandle pipeline, ShaderType shaderType, const char* name, const GraphicsHandle buffer)
{
    auto& backend = GetBackend();
    backend.SetStaticVariable_Impl(pipeline, shaderType, name, buffer);
}

void Graphics::SetPipeline(const GraphicsHandle pipeline)
{
    auto& backend = GetBackend();
    backend.SetPipeline_Impl(pipeline);
}

void Graphics::SetVertexBuffers(i32 i, i32 count, const GraphicsHandle* pBuffers, const u64* offset)
{
    auto& backend = GetBackend();
    backend.SetVertexBuffers_Impl(i, count, pBuffers, offset);
}

void Graphics::SetIndexBuffer(const GraphicsHandle buffer, i32 i)
{
    auto& backend = GetBackend();
    backend.SetIndexBuffer_Impl(buffer, i);
}

void Graphics::Draw(const DrawAttribs& attribs)
{
    auto& backend = GetBackend();
    backend.Draw_Impl(attribs);
}

void Graphics::DrawIndexed(const DrawIndexedAttribs& attribs)
{
    auto& backend = GetBackend();
    backend.DrawIndexed_Impl(attribs);
}*/