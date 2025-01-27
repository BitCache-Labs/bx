#pragma once

#include <engine/api.hpp>
#include <engine/graphics.hpp>
#include <engine/macros.hpp>
#include <engine/enum.hpp>
#include <engine/string.hpp>
#include <engine/hash_map.hpp>
#include <engine/window.hpp>
#include <engine/math.hpp>

#include <engine/opengl/gl_common.hpp>

//#include <rttr/registration.h>
//RTTR_PLUGIN_REGISTRATION
//{
//    rttr::registration::class_<GraphicsOpenGLES>("GraphicsOpenGLES")
//        .constructor();
//}

#ifdef EDITOR_BUILD
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#endif

#define GLSL_VERSION "#version 310 es\n"

#define GLSL_VERT_SHADER GLSL_VERSION "#define VERTEX\n"
#define GLSL_FRAG_SHADER GLSL_VERSION "#define PIXEL\n"

#define GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX            0x9047
#define GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX      0x9048
#define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX    0x9049
#define GPU_MEMORY_INFO_EVICTION_COUNT_NVX              0x904A
#define GPU_MEMORY_INFO_EVICTED_MEMORY_NVX              0x904B

#define VBO_FREE_MEMORY_ATI                             0x87FB
#define TEXTURE_FREE_MEMORY_ATI                         0x87FC
#define RENDERBUFFER_FREE_MEMORY_ATI                    0x87FD

#define MAX_LAYOUT_ELEMS                                16
#define MAX_BOUND_VERTEX_BUFFERS                        16

struct BX_API ShaderImpl
{
    GLuint handle = 0;
};

struct BX_API BufferImpl
{
    GLuint handle = 0;
    GLenum target = 0;
    GLenum usage = 0;
    GLsizei stride = 0;
};

struct BX_API TextureImpl
{
    GLuint texture = 0;
    GLuint sampler = 0;
    GLuint fbo = 0;
    GLuint rbo = 0;

    GLuint64 handle = 0;
};

struct BX_API ResourceBindingImpl
{
    struct Data
    {
        ShaderType shaderType = ShaderType::UNKNOWN;
        u32 count = 0;
        ResourceBindingType type = ResourceBindingType::UNKNOWN;
        ResourceBindingAccess access = ResourceBindingAccess::STATIC;

        GraphicsHandle handle = INVALID_GRAPHICS_HANDLE;
    };
    HashMap<String, Data> resources;
};

struct BX_API PipelineImpl
{
    GLuint program = 0;
    GLuint vao = 0;

    GraphicsHandle prevVertBuffer = INVALID_GRAPHICS_HANDLE;
    LayoutElement layoutElements[MAX_LAYOUT_ELEMS];
    u32 numElements = 0;

    PipelineTopology topology = PipelineTopology::TRIANGLES;
    PipelineFaceCull faceCull = PipelineFaceCull::CCW;

    bool depthEnable = true;
    bool blendEnable = true;

    GLuint bufferCount = 0;
};

class BX_API GraphicsOpenGLES final : public Graphics
{
    BX_MODULE(GraphicsOpenGLES, Graphics)
    friend class GraphicsOpenGLESEditor;

public:
    bool Initialize() override;
    void Shutdown() override;

    void NewFrame() override;
    void EndFrame() override;

    TextureFormat GetColorBufferFormat() override;
    TextureFormat GetDepthBufferFormat() override;

    GraphicsHandle GetCurrentBackBufferRT() override;
    GraphicsHandle GetDepthBuffer() override;

    void SetRenderTarget(const GraphicsHandle renderTarget, const GraphicsHandle depthStencil) override;
    void ReadPixels(u32 x, u32 y, u32 w, u32 h, void* pixelData, const GraphicsHandle renderTarget) override;

    void SetViewport(const f32 viewport[4]) override;

    void ClearRenderTarget(const GraphicsHandle renderTarget, const f32 clearColor[4]) override;
    void ClearDepthStencil(const GraphicsHandle depthStencil, GraphicsClearFlags flags, f32 depth, i32 stencil) override;

    GraphicsHandle CreateShader(const ShaderInfo& info) override;
    void DestroyShader(const GraphicsHandle shader) override;

    GraphicsHandle CreateTexture(const TextureInfo& info, const BufferData& data) override;
    void DestroyTexture(const GraphicsHandle texture) override;

    GraphicsHandle CreateResourceBinding(const ResourceBindingInfo& info) override;
    void DestroyResourceBinding(const GraphicsHandle resources) override;
    void BindResource(const GraphicsHandle resources, const char* name, GraphicsHandle resource) override;

    GraphicsHandle CreatePipeline(const PipelineInfo& info) override;
    void DestroyPipeline(const GraphicsHandle pipeline) override;
    void SetPipeline(const GraphicsHandle pipeline) override;
    void SetUniform(const GraphicsHandle pipeline, StringView name, GraphicsValueType valueType, u32 count, u8* data) override;
    void CommitResources(const GraphicsHandle pipeline, const GraphicsHandle resources) override;

    GraphicsHandle CreateBuffer(const BufferInfo& info, const BufferData& data) override;
    void DestroyBuffer(const GraphicsHandle buffer) override;
    void UpdateBuffer(const GraphicsHandle buffer, const BufferData& data) override;

    void SetVertexBuffers(i32 i, i32 count, const GraphicsHandle* pBuffers, const u64* offset) override;
    void SetIndexBuffer(const GraphicsHandle buffer, i32 i) override;

    void Draw(const DrawAttribs& attribs) override;
    void DrawIndexed(const DrawIndexedAttribs& attribs) override;

#ifdef EDITOR_BUILD
    bool InitializeImGui() override
    {
        ImGui_ImplOpenGL3_Init("#version 300 es");
        return true;
    }

    void ShutdownImGui() override
    {
        ImGui_ImplOpenGL3_Shutdown();
    }

    void NewFrameImGui() override
    {
        ImGui_ImplOpenGL3_NewFrame();
    }

    void EndFrameImGui() override
    {
        ImGui::Render();

        //GraphicsHandle renderTarget = GetCurrentBackBufferRT();
        //GraphicsHandle depthStencil = GetDepthBuffer();
        //SetRenderTarget(renderTarget, depthStencil);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
#endif

public:
    u32 GetTextureHandle(GraphicsHandle texture);

private:
    GraphicsContext m_ctx{};

    HashMap<GraphicsHandle, ShaderImpl> m_shaders;
    HashMap<GraphicsHandle, BufferImpl> m_buffers;
    HashMap<GraphicsHandle, TextureImpl> m_textures;
    HashMap<GraphicsHandle, ResourceBindingImpl> m_resources;
    HashMap<GraphicsHandle, PipelineImpl> m_pipelines;
};