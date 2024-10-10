#include "bx/platform/graphics.hpp"
#include "bx/platform/file.hpp"

#include <bx/core/log.hpp>
#include <bx/core/time.hpp>
#include <bx/core/guard.hpp>
#include <bx/core/profiler.hpp>
#include <bx/containers/pool.hpp>

#include <rttr/type>
#include <rttr/registration>
#include <stdexcept>
#include <iostream>

Graphics& Graphics::Get()
{
    static Graphics* instance = nullptr;
    if (!instance)
    {
        const auto& derived = rttr::type::get<Graphics>().get_derived_classes();
        if (derived.size() == 0)
            throw std::runtime_error("No derived Graphics class found.");

        const auto& type = *derived.begin();
        rttr::variant var = type.create();

        BX_ENSURE(var.is_type<Graphics*>());
        instance = var.get_value<Graphics*>();
    }
    return *instance;
}

void Graphics::DebugLine(const Vec3& a, const Vec3& b, u32 color, f32 lifespan)
{
    m_debugLines.emplace_back(a, b, color, lifespan);
}

void Graphics::UpdateDebugLines()
{
    m_debugVertices.clear();

    m_debugLinesBuffer.clear();
    m_debugLinesBuffer.reserve(m_debugLines.size());
    for (auto& line : m_debugLines)
    {
        m_debugVertices.emplace_back(line.a, line.color);
        m_debugVertices.emplace_back(line.b, line.color);

        line.lifespan -= Time::GetDeltaTime();
        if (line.lifespan > 0.0f)
            m_debugLinesBuffer.emplace_back(line);
    }
    m_debugLines = m_debugLinesBuffer;
}

void Graphics::DrawDebugLines(const Mat4& viewProj)
{
    DebugDrawAttribs attribs;
    DebugDraw(viewProj, attribs, m_debugVertices);
}

void Graphics::ClearDebugLines()
{
    m_debugLines.clear();
}

RTTR_PLUGIN_REGISTRATION
{
    /*
    rttr::registration::enumeration<GraphicsClearFlags>("GraphicsClearFlags")
    (
        rttr::value("NONE", GraphicsClearFlags::NONE),
        rttr::value("DEPTH", GraphicsClearFlags::DEPTH),
        rttr::value("STENCIL", GraphicsClearFlags::STENCIL)
    );

    rttr::registration::enumeration<GraphicsValueType>("GraphicsValueType")
    (
        rttr::value("UNDEFINED", GraphicsValueType::UNDEFINED),
        rttr::value("INT8", GraphicsValueType::INT8),
        rttr::value("INT16", GraphicsValueType::INT16),
        rttr::value("INT32", GraphicsValueType::INT32),
        rttr::value("UINT8", GraphicsValueType::UINT8),
        rttr::value("UINT16", GraphicsValueType::UINT16),
        rttr::value("UINT32", GraphicsValueType::UINT32),
        rttr::value("FLOAT16", GraphicsValueType::FLOAT16),
        rttr::value("FLOAT32", GraphicsValueType::FLOAT32)
    );

    rttr::registration::enumeration<ShaderType>("ShaderType")
    (
        rttr::value("UNKNOWN", ShaderType::UNKNOWN),
        rttr::value("VERTEX", ShaderType::VERTEX),
        rttr::value("PIXEL", ShaderType::PIXEL),
        rttr::value("GEOMETRY", ShaderType::GEOMETRY),
        rttr::value("COMPUTE", ShaderType::COMPUTE)
    );

    rttr::registration::enumeration<BufferType>("BufferType")
    (
        rttr::value("VERTEX_BUFFER", BufferType::VERTEX_BUFFER),
        rttr::value("INDEX_BUFFER", BufferType::INDEX_BUFFER),
        rttr::value("UNIFORM_BUFFER", BufferType::UNIFORM_BUFFER),
        rttr::value("STORAGE_BUFFER", BufferType::STORAGE_BUFFER)
    );

    rttr::registration::enumeration<BufferUsage>("BufferUsage")
    (
        rttr::value("IMMUTABLE", BufferUsage::IMMUTABLE),
        rttr::value("DEFAULT", BufferUsage::DEFAULT),
        rttr::value("DYNAMIC", BufferUsage::DYNAMIC)
    );

    rttr::registration::enumeration<BufferAccess>("BufferAccess")
    (
        rttr::value("NONE", BufferAccess::NONE),
        rttr::value("READ", BufferAccess::READ),
        rttr::value("WRITE", BufferAccess::WRITE)
    );

    rttr::registration::enumeration<TextureFormat>("TextureFormat")
    (
        rttr::value("UNKNOWN", TextureFormat::UNKNOWN),
        rttr::value("RGB8_UNORM", TextureFormat::RGB8_UNORM),
        rttr::value("RGBA8_UNORM", TextureFormat::RGBA8_UNORM),
        rttr::value("RG32_UINT", TextureFormat::RG32_UINT),
        rttr::value("D24_UNORM_S8_UINT", TextureFormat::D24_UNORM_S8_UINT)
    );

    rttr::registration::enumeration<TextureFlags>("TextureFlags")
    (
        rttr::value("NONE", TextureFlags::NONE),
        rttr::value("SHADER_RESOURCE", TextureFlags::SHADER_RESOURCE),
        rttr::value("RENDER_TARGET", TextureFlags::RENDER_TARGET),
        rttr::value("DEPTH_STENCIL", TextureFlags::DEPTH_STENCIL)
    );

    rttr::registration::enumeration<ResourceBindingType>("ResourceBindingType")
    (
        rttr::value("UNKNOWN", ResourceBindingType::UNKNOWN),
        rttr::value("TEXTURE", ResourceBindingType::TEXTURE),
        rttr::value("UNIFORM_BUFFER", ResourceBindingType::UNIFORM_BUFFER),
        rttr::value("STORAGE_BUFFER", ResourceBindingType::STORAGE_BUFFER)
    );

    rttr::registration::enumeration<ResourceBindingAccess>("ResourceBindingAccess")
    (
        rttr::value("STATIC", ResourceBindingAccess::STATIC),
        rttr::value("MUTABLE", ResourceBindingAccess::MUTABLE),
        rttr::value("DYNAMIC", ResourceBindingAccess::DYNAMIC)
    );

    rttr::registration::enumeration<PipelineTopology>("PipelineTopology")
    (
        rttr::value("UNDEFINED", PipelineTopology::UNDEFINED),
        rttr::value("POINTS", PipelineTopology::POINTS),
        rttr::value("LINES", PipelineTopology::LINES),
        rttr::value("TRIANGLES", PipelineTopology::TRIANGLES)
    );

    rttr::registration::enumeration<PipelineFaceCull>("PipelineFaceCull")
    (
        rttr::value("NONE", PipelineFaceCull::NONE),
        rttr::value("CW", PipelineFaceCull::CW),
        rttr::value("CCW", PipelineFaceCull::CCW)
    );

    rttr::registration::class_<ShaderInfo>("ShaderInfo")
        .constructor<>()
        .property("shaderType", &ShaderInfo::shaderType)
        .property("source", &ShaderInfo::source);

    rttr::registration::class_<BufferInfo>("BufferInfo")
        .constructor<>()
        .property("strideBytes", &BufferInfo::strideBytes)
        .property("type", &BufferInfo::type)
        .property("usage", &BufferInfo::usage)
        .property("access", &BufferInfo::access);

    rttr::registration::class_<TextureInfo>("TextureInfo")
        .constructor<>()
        .property("format", &TextureInfo::format)
        .property("width", &TextureInfo::width)
        .property("height", &TextureInfo::height)
        .property("flags", &TextureInfo::flags);

    rttr::registration::class_<BufferData>("BufferData")
        .constructor<>()
        .property("pData", &BufferData::pData)
        .property("dataSize", &BufferData::dataSize);

    rttr::registration::class_<ResourceBindingElement>("ResourceBindingElement")
        .constructor<>()
        .property("shaderType", &ResourceBindingElement::shaderType)
        .property("name", &ResourceBindingElement::name)
        .property("count", &ResourceBindingElement::count)
        .property("type", &ResourceBindingElement::type)
        .property("access", &ResourceBindingElement::access);

    rttr::registration::class_<ResourceBindingInfo>("ResourceBindingInfo")
        .constructor<>()
        .property("resources", &ResourceBindingInfo::resources)
        .property("numResources", &ResourceBindingInfo::numResources);

    rttr::registration::class_<LayoutElement>("LayoutElement")
        .constructor<>()
        .property("inputIndex", &LayoutElement::inputIndex)
        .property("bufferSlot", &LayoutElement::bufferSlot)
        .property("numComponents", &LayoutElement::numComponents)
        .property("valueType", &LayoutElement::valueType)
        .property("isNormalized", &LayoutElement::isNormalized)
        .property("relativeOffset", &LayoutElement::relativeOffset)
        .property("instanceDataStepRate", &LayoutElement::instanceDataStepRate);

    rttr::registration::class_<PipelineInfo>("PipelineInfo")
        .constructor<>()
        .property("numRenderTargets", &PipelineInfo::numRenderTargets)
        .property("renderTargetFormats", &PipelineInfo::renderTargetFormats)
        .property("depthStencilFormat", &PipelineInfo::depthStencilFormat)
        .property("topology", &PipelineInfo::topology)
        .property("faceCull", &PipelineInfo::faceCull)
        .property("depthEnable", &PipelineInfo::depthEnable)
        .property("blendEnable", &PipelineInfo::blendEnable)
        .property("vertShader", &PipelineInfo::vertShader)
        .property("pixelShader", &PipelineInfo::pixelShader)
        .property("layoutElements", &PipelineInfo::layoutElements)
        .property("numElements", &PipelineInfo::numElements);

    rttr::registration::class_<DrawAttribs>("DrawAttribs")
        .constructor<>()
        .property("numVertices", &DrawAttribs::numVertices);

    rttr::registration::class_<DrawIndexedAttribs>("DrawIndexedAttribs")
        .constructor<>()
        .property("indexType", &DrawIndexedAttribs::indexType)
        .property("numIndices", &DrawIndexedAttribs::numIndices);

    rttr::registration::class_<DebugVertex>("DebugVertex")
        .constructor<>()
        .property("vert", &DebugVertex::vert)
        .property("col", &DebugVertex::col);

    rttr::registration::class_<DebugDrawAttribs>("DebugDrawAttribs")
        .constructor<>();

    rttr::registration::class_<Graphics>("Graphics")
        .method("Initialize", &Graphics::Initialize)
        .method("Reload", &Graphics::Reload)
        .method("Shutdown", &Graphics::Shutdown)
        .method("NewFrame", &Graphics::NewFrame)
        .method("EndFrame", &Graphics::EndFrame)
        .method("GetColorBufferFormat", &Graphics::GetColorBufferFormat)
        .method("GetDepthBufferFormat", &Graphics::GetDepthBufferFormat)
        .method("GetCurrentBackBufferRT", &Graphics::GetCurrentBackBufferRT)
        .method("GetDepthBuffer", &Graphics::GetDepthBuffer)
        .method("SetRenderTarget", &Graphics::SetRenderTarget)
        .method("ReadPixels", &Graphics::ReadPixels)
        .method("SetViewport", &Graphics::SetViewport)
        .method("ClearRenderTarget", &Graphics::ClearRenderTarget)
        .method("ClearDepthStencil", &Graphics::ClearDepthStencil)
        .method("CreateShader", rttr::select_overload<GraphicsHandle(const ShaderInfo&)>(&Graphics::CreateShader))
        .method("DestroyShader", &Graphics::DestroyShader)
        .method("CreateTexture", rttr::select_overload<GraphicsHandle(const TextureInfo&)>(&Graphics::CreateTexture))
        .method("CreateTexture", rttr::select_overload<GraphicsHandle(const TextureInfo&, const BufferData&)>(&Graphics::CreateTexture))
        .method("DestroyTexture", &Graphics::DestroyTexture)
        .method("CreateResourceBinding", &Graphics::CreateResourceBinding)
        .method("DestroyResourceBinding", &Graphics::DestroyResourceBinding)
        .method("BindResource", &Graphics::BindResource)
        .method("CreatePipeline", &Graphics::CreatePipeline)
        .method("DestroyPipeline", &Graphics::DestroyPipeline)
        .method("SetPipeline", &Graphics::SetPipeline)
        .method("CommitResources", &Graphics::CommitResources)
        .method("CreateBuffer", rttr::select_overload<GraphicsHandle(const BufferInfo&)>(&Graphics::CreateBuffer))
        .method("CreateBuffer", rttr::select_overload<GraphicsHandle(const BufferInfo&, const BufferData&)>(&Graphics::CreateBuffer))
        .method("DestroyBuffer", &Graphics::DestroyBuffer)
        .method("UpdateBuffer", &Graphics::UpdateBuffer)
        .method("SetVertexBuffers", &Graphics::SetVertexBuffers)
        .method("SetIndexBuffer", &Graphics::SetIndexBuffer)
        .method("Draw", &Graphics::Draw)
        .method("DrawIndexed", &Graphics::DrawIndexed)
        .method("DebugLine", &Graphics::DebugLine)
        .method("UpdateDebugLines", &Graphics::UpdateDebugLines)
        .method("DrawDebugLines", &Graphics::DrawDebugLines)
        .method("ClearDebugLines", &Graphics::ClearDebugLines)
        .method("DebugDraw", &Graphics::DebugDraw);
        */

    rttr::registration::class_<Graphics>("Graphics");
}