

#include "bx/engine/modules/graphics/backend/graphics_opengl.hpp"

#include "bx/engine/modules/graphics/type_validation.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/engine/core/profiler.hpp"
#include "bx/engine/core/memory.hpp"
#include "bx/engine/core/macros.hpp"
#include "bx/engine/containers/array.hpp"

#include "bx/engine/modules/window.hpp"
#include "bx/engine/modules/imgui.hpp"

#include "bx/engine/modules/graphics/backend/opengl/conversion.hpp"
#include "bx/engine/modules/graphics/backend/opengl/graphics_pipeline.hpp"
#include "bx/engine/modules/graphics/backend/opengl/shader.hpp"
#include "bx/engine/modules/graphics/backend/opengl/validation.hpp"
using namespace Gl;

#ifdef BX_WINDOW_GLFW_BACKEND
#include "bx/engine/modules/window/backend/window_glfw.hpp"
#endif

struct TextureView
{
    TextureHandle handle;
    GLuint texture;
};

struct GraphicsBackendState : NoCopy
{
    HandlePool<BufferApi> bufferHandlePool;
    HandlePool<TextureApi> textureHandlePool;
    HandlePool<TextureViewApi> textureViewHandlePool;
    HandlePool<SamplerApi> samplerHandlePool;
    HandlePool<ShaderApi> shaderHandlePool;
    HandlePool<GraphicsPipelineApi> graphicsPipelineHandlePool;
    HandlePool<ComputePipelineApi> computePipelineHandlePool;
    HandlePool<RenderPassApi> renderPassHandlePool;
    HandlePool<ComputePassApi> computePassHandlePool;
    HandlePool<BindGroupApi> bindGroupHandlePool;

    HashMap<TextureHandle, GLuint> textures;
    HashMap<TextureViewHandle, TextureView> textureViews;
    HashMap<BufferHandle, GLuint> buffers;
    HashMap<ShaderHandle, Shader> shaders;
    HashMap<GraphicsPipelineHandle, GraphicsPipeline> graphicsPipelines;
    HashMap<ComputePipelineHandle, ShaderProgram> computePipelines;
    GLuint framebuffer;
    GLuint readbackFramebuffer;

    RenderPassHandle activeRenderPass = RenderPassHandle::null;
    ComputePassHandle activeComputePass = ComputePassHandle::null;
    GraphicsPipelineHandle boundGraphicsPipeline = GraphicsPipelineHandle::null;
    ComputePipelineHandle boundComputePipeline = ComputePipelineHandle::null;
    Optional<IndexFormat> boundIndexFormat = Optional<IndexFormat>::None();

    BufferHandle emptyBuffer = BufferHandle::null;
    TextureHandle emptyTexture = TextureHandle::null;

    TextureHandle swapchainColorTarget = TextureHandle::null;
    TextureViewHandle swapchainColorTargetView = TextureViewHandle::null;
};
static std::unique_ptr<GraphicsBackendState> s;

b8 Graphics::Initialize()
{
    s_createInfoCache = std::make_unique<CreateInfoCache>();
    s = std::make_unique<GraphicsBackendState>();

    Gl::Init(false);

    BufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.name = "Empty Buffer";
    bufferCreateInfo.size = 1;
    bufferCreateInfo.usageFlags = BufferUsageFlags::COPY_SRC | BufferUsageFlags::INDEX | BufferUsageFlags::VERTEX | BufferUsageFlags::UNIFORM | BufferUsageFlags::STORAGE;
    s->emptyBuffer = Graphics::CreateBuffer(bufferCreateInfo);

    TextureCreateInfo textureCreateInfo{};
    textureCreateInfo.name = "Empty Texture";
    textureCreateInfo.size = Extend3D(1, 1, 1);
    textureCreateInfo.usageFlags = TextureUsageFlags::COPY_SRC | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::STORAGE_BINDING;

    glCreateFramebuffers(1, &s->framebuffer);
    glCreateFramebuffers(1, &s->readbackFramebuffer);

    i32 w, h;
    Window::GetSize(&w, &h);

    TextureCreateInfo swapchainColorTargetCreateInfo{};
    swapchainColorTargetCreateInfo.name = "Swapchain Color Target";
    swapchainColorTargetCreateInfo.size = Extend3D(w, h, 1);
    swapchainColorTargetCreateInfo.usageFlags = TextureUsageFlags::COPY_SRC | TextureUsageFlags::COPY_DST | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::RENDER_ATTACHMENT;
    s->swapchainColorTarget = Graphics::CreateTexture(swapchainColorTargetCreateInfo);
    s->swapchainColorTargetView = Graphics::CreateTextureView(s->swapchainColorTarget);

    return true;
}

void Graphics::Reload()
{

}

void Graphics::Shutdown()
{
    // TODO: clear a shitload of gl objects (textures, buffers, etc.)

    glDeleteFramebuffers(1, &s->framebuffer);
    glDeleteFramebuffers(1, &s->readbackFramebuffer);

    s.reset();
}

void Graphics::NewFrame()
{
    if (Window::WasResized())
    {
        i32 w, h;
        Window::GetSize(&w, &h);

        TextureCreateInfo swapchainColorTargetCreateInfo{};
        swapchainColorTargetCreateInfo.name = "Swapchain Color Target";
        swapchainColorTargetCreateInfo.size = Extend3D(w, h, 1);
        swapchainColorTargetCreateInfo.format = TextureFormat::RGBA8_UNORM;
        swapchainColorTargetCreateInfo.usageFlags = TextureUsageFlags::COPY_SRC | TextureUsageFlags::COPY_DST | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::RENDER_ATTACHMENT;
        if (s->swapchainColorTarget) Graphics::DestroyTexture(s->swapchainColorTarget);
        s->swapchainColorTarget = Graphics::CreateTexture(swapchainColorTargetCreateInfo);

        if (s->swapchainColorTargetView) Graphics::DestroyTextureView(s->swapchainColorTargetView);
        s->swapchainColorTargetView = Graphics::CreateTextureView(s->swapchainColorTarget);
    }
}

void Graphics::EndFrame()
{
    BX_ASSERT(s->activeRenderPass == RenderPassHandle::null, "Render pass must have been ended before the end of the frame.");
    BX_ASSERT(s->activeComputePass == ComputePassHandle::null, "Compute pass must have been ended before the end of the frame.");

    s->boundGraphicsPipeline = GraphicsPipelineHandle::null;
    s->boundComputePipeline = ComputePipelineHandle::null;
    s->boundIndexFormat = Optional<IndexFormat>::None();

    glNamedFramebufferTexture(s->framebuffer, GL_COLOR_ATTACHMENT0, s->swapchainColorTarget, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, s->framebuffer);

    i32 w, h;
    Window::GetSize(&w, &h);
    glBlitNamedFramebuffer(s->framebuffer, 0,
        0, 0, w, h, 0, 0, w, h,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGuiImpl::EndFrame();
}

const BufferHandle& Graphics::EmptyBuffer()
{
    return s->emptyBuffer;
}

const TextureHandle& Graphics::EmptyTexture()
{
    return s->emptyTexture;
}

TextureHandle Graphics::GetSwapchainColorTarget()
{
    return s->swapchainColorTarget;
}

TextureViewHandle Graphics::GetSwapchainColorTargetView()
{
    return s->swapchainColorTargetView;
}

TextureHandle Graphics::CreateTexture(const TextureCreateInfo& createInfo)
{
    return CreateTexture(createInfo, nullptr);
}

TextureHandle Graphics::CreateTexture(const TextureCreateInfo& createInfo, const void* data)
{
    BX_ENSURE(ValidateTextureCreateInfo(createInfo));

    TextureHandle textureHandle = s->textureHandlePool.Create();
    s_createInfoCache->textureCreateInfos.insert(std::make_pair(textureHandle, createInfo));

    GLenum type = TextureDimensionToGl(createInfo.dimension, createInfo.size.depthOrArrayLayers);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(type, texture);

    if (type == GL_TEXTURE_1D)
    {
        // TODO
        BX_FAIL("TODO");
    }
    else if (type == GL_TEXTURE_2D || type == GL_TEXTURE_1D_ARRAY)
    {
        u32 height = (type == GL_TEXTURE_1D_ARRAY) ? createInfo.size.depthOrArrayLayers : createInfo.size.height;

        if (createInfo.format == TextureFormat::RG32_UINT)
        {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                TextureFormatToGlInternalFormat(createInfo.format),
                createInfo.size.width,
                height,
                0,
                TextureFormatToGlFormat(createInfo.format),
                TextureFormatToGlType(createInfo.format),
                data
            );
        }
        else
        {
            glTexImage2D(
                type,
                0,
                TextureFormatToGlInternalFormat(createInfo.format),
                createInfo.size.width,
                height,
                0,
                TextureFormatToGlFormat(createInfo.format),
                TextureFormatToGlType(createInfo.format),
                data
            );
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Required, default enables mips
    }
    else if (type == GL_TEXTURE_3D || type == GL_TEXTURE_2D_ARRAY)
    {
        glTexImage3D(
            type,
            0,
            TextureFormatToGlInternalFormat(createInfo.format),
            createInfo.size.width,
            createInfo.size.height,
            createInfo.size.depthOrArrayLayers,
            0,
            TextureFormatToGlFormat(createInfo.format),
            TextureFormatToGlType(createInfo.format),
            data
        );
    }

    glBindTexture(type, 0);

    s->textures.insert(std::make_pair(textureHandle, texture));

    return textureHandle;
}

void Graphics::DestroyTexture(TextureHandle& texture)
{
    BX_ENSURE(texture);

    auto& textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());
    glDeleteTextures(1, &textureIter->second);

    s->textures.erase(texture);
    s_createInfoCache->textureCreateInfos.erase(texture);
    s->textureHandlePool.Destroy(texture);
}

TextureViewHandle Graphics::CreateTextureView(TextureHandle texture)
{
    BX_ENSURE(texture);

    TextureViewHandle textureViewHandle = s->textureViewHandlePool.Create();
    
    auto& textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());

    TextureView textureView;
    textureView.handle = texture;
    textureView.texture = textureIter->second;

    s->textureViews.insert(std::make_pair(textureViewHandle, textureView));

    return textureViewHandle;
}

void Graphics::DestroyTextureView(TextureViewHandle& textureView)
{
    BX_ENSURE(textureView);

    s->textureViews.erase(textureView);
    s->textureViewHandlePool.Destroy(textureView);
}

SamplerHandle Graphics::CreateSampler(const SamplerCreateInfo& create)
{
    BX_FAIL("TODO");
    return SamplerHandle::null;
}

void Graphics::DestroySampler(SamplerHandle& sampler)
{

}

BufferHandle Graphics::CreateBuffer(const BufferCreateInfo& createInfo)
{
    return CreateBuffer(createInfo, nullptr);
}

BufferHandle Graphics::CreateBuffer(const BufferCreateInfo& createInfo, const void* data)
{
    BX_ENSURE(ValidateBufferCreateInfo(createInfo));

    BufferHandle bufferHandle = s->bufferHandlePool.Create();
    s_createInfoCache->bufferCreateInfos.insert(std::make_pair(bufferHandle, createInfo));

    GLuint buffer;
    glCreateBuffers(1, &buffer);
    glNamedBufferData(buffer, createInfo.size, data, GL_DYNAMIC_DRAW);

    s->buffers.insert(std::make_pair(bufferHandle, buffer));

    return bufferHandle;
}

void Graphics::DestroyBuffer(BufferHandle& buffer)
{
    BX_ENSURE(buffer);

    auto& bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());
    glDeleteBuffers(1, &bufferIter->second);

    s->buffers.erase(buffer);
    s_createInfoCache->bufferCreateInfos.erase(buffer);
    s->bufferHandlePool.Destroy(buffer);
}

ShaderHandle Graphics::CreateShader(const ShaderCreateInfo& createInfo)
{
    BX_ENSURE(ValidateShaderCreateInfo(createInfo));

    ShaderHandle shaderHandle = s->shaderHandlePool.Create();
    s_createInfoCache->shaderCreateInfos.insert(std::make_pair(shaderHandle, createInfo));

    String meta = String("#version 450\n");
    switch (createInfo.shaderType)
    {
    case ShaderType::VERTEX:
    {
        meta += String("#define VERTEX\n");
        break;
    }
    case ShaderType::FRAGMENT:
    {
        meta += String("#define FRAGMENT\n");
        break;
    }
    }

    GLenum type = ShaderTypeToGl(createInfo.shaderType);
    s->shaders.try_emplace(shaderHandle, createInfo.name, type, meta + createInfo.src);

    return shaderHandle;
}

void Graphics::DestroyShader(ShaderHandle& shader)
{
    BX_ENSURE(shader);

    auto& shaderIter = s->shaders.find(shader);
    BX_ENSURE(shaderIter != s->shaders.end());

    s->shaders.erase(shader);
    s_createInfoCache->shaderCreateInfos.erase(shader);
    s->shaderHandlePool.Destroy(shader);
}

GraphicsPipelineHandle Graphics::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
{
    BX_ENSURE(ValidateGraphicsPipelineCreateInfo(createInfo));

    GraphicsPipelineHandle graphicsPipelineHandle = s->graphicsPipelineHandlePool.Create();
    s_createInfoCache->graphicsPipelineCreateInfos.insert(std::make_pair(graphicsPipelineHandle, createInfo));

    auto& vertShaderIter = s->shaders.find(createInfo.vertexShader);
    BX_ENSURE(vertShaderIter != s->shaders.end());
    auto& fragShaderIter = s->shaders.find(createInfo.fragmentShader);
    BX_ENSURE(fragShaderIter != s->shaders.end());

    ShaderProgram shaderProgram(createInfo.name, List<Shader*>{ &vertShaderIter->second, & fragShaderIter->second });
    s->graphicsPipelines.try_emplace(graphicsPipelineHandle, std::move(shaderProgram), createInfo.vertexBuffers, createInfo.layout);

    return graphicsPipelineHandle;
}

void Graphics::DestroyGraphicsPipeline(GraphicsPipelineHandle& graphicsPipeline)
{
    BX_ENSURE(graphicsPipeline);

    s->graphicsPipelines.erase(graphicsPipeline);
    s_createInfoCache->graphicsPipelineCreateInfos.erase(graphicsPipeline);
    s->graphicsPipelineHandlePool.Destroy(graphicsPipeline);
}

ComputePipelineHandle Graphics::CreateComputePipeline(const ComputePipelineCreateInfo& createInfo)
{
    BX_ENSURE(ValidateComputePipelineCreateInfo(createInfo));

    ComputePipelineHandle computePipelineHandle = s->computePipelineHandlePool.Create();
    s_createInfoCache->computePipelineCreateInfos.insert(std::make_pair(computePipelineHandle, createInfo));

    auto& shaderIter = s->shaders.find(createInfo.shader);
    BX_ENSURE(shaderIter != s->shaders.end());

    s->computePipelines.emplace(std::make_pair(computePipelineHandle, std::move(ShaderProgram(createInfo.name, List<Shader*>{ &shaderIter->second }))));

    return computePipelineHandle;
}

void Graphics::DestroyComputePipeline(ComputePipelineHandle& computePipeline)
{
    BX_ENSURE(computePipeline);

    s->computePipelines.erase(computePipeline);
    s_createInfoCache->computePipelineCreateInfos.erase(computePipeline);
    s->computePipelineHandlePool.Destroy(computePipeline);
}

BindGroupLayoutHandle Graphics::GetBindGroupLayout(GraphicsPipelineHandle graphicsPipeline, u32 bindGroup)
{
    u64 id = graphicsPipeline.id << 10 | static_cast<u64>(static_cast<u8>(bindGroup)) << 1 | 1;
    return BindGroupLayoutHandle{ id };
}

BindGroupLayoutHandle Graphics::GetBindGroupLayout(ComputePipelineHandle computePipeline, u32 bindGroup)
{
    u64 id = computePipeline.id << 10 | static_cast<u64>(static_cast<u8>(bindGroup)) << 1 | 0;
    return BindGroupLayoutHandle{ id };
}

u32 Graphics::GetBindGroupLayoutBindGroup(BindGroupLayoutHandle bindGroupLayout)
{
    return static_cast<u32>((bindGroupLayout.id << 54) >> 55);
}

b8 Graphics::IsBindGroupLayoutGraphics(BindGroupLayoutHandle bindGroupLayout)
{
    return static_cast<b8>(bindGroupLayout.id << 63);
}

u64 Graphics::GetBindGroupLayoutPipeline(BindGroupLayoutHandle bindGroupLayout)
{
    return bindGroupLayout.id >> 10;
}

BindGroupHandle Graphics::CreateBindGroup(const BindGroupCreateInfo& createInfo)
{
    BX_ENSURE(ValidateBindGroupCreateInfo(createInfo));

    BindGroupHandle bindGroupHandle = s->bindGroupHandlePool.Create();
    s_createInfoCache->bindGroupCreateInfos.insert(std::make_pair(bindGroupHandle, createInfo));

    return bindGroupHandle;
}

void Graphics::DestroyBindGroup(BindGroupHandle& bindGroup)
{
    BX_ENSURE(bindGroup);

    s_createInfoCache->bindGroupCreateInfos.erase(bindGroup);
    s->bindGroupHandlePool.Destroy(bindGroup);
}

RenderPassHandle Graphics::BeginRenderPass(const RenderPassDescriptor& descriptor)
{
    BX_ASSERT(!s->activeRenderPass, "Render pass already active.");
    
    u32 width = 0;
    u32 height = 0;

    for (u32 i = 0; i < descriptor.colorAttachments.size(); i++)
    {
        const RenderPassColorAttachment& attachment = descriptor.colorAttachments[i];

        auto& textureViewIter = s->textureViews.find(attachment.view);
        BX_ENSURE(textureViewIter != s->textureViews.end());

        glNamedFramebufferTexture(s->framebuffer, GL_COLOR_ATTACHMENT0 + i, textureViewIter->second.texture, 0);

        if (width == 0 && height == 0)
        {
            auto createInfo = GetTextureCreateInfo(textureViewIter->second.handle);
            width = createInfo.size.width;
            height = createInfo.size.height;
        }
    }

    if (descriptor.depthStencilAttachment.IsSome())
    {
        const RenderPassDepthStencilAttachment& attachment = descriptor.depthStencilAttachment.Unwrap();

        auto& textureViewIter = s->textureViews.find(attachment.view);
        BX_ENSURE(textureViewIter != s->textureViews.end());

        glNamedFramebufferTexture(s->framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, textureViewIter->second.texture, 0);

        if (width == 0 && height == 0)
        {
            auto createInfo = GetTextureCreateInfo(textureViewIter->second.handle);
            width = createInfo.size.width;
            height = createInfo.size.height;
        }
    }

    BX_ENSURE(width != 0 && height != 0);

    glBindFramebuffer(GL_FRAMEBUFFER, s->framebuffer);

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    RenderPassHandle renderPassHandle = s->renderPassHandlePool.Create();
    s_createInfoCache->renderPassCreateInfos.insert(std::make_pair(renderPassHandle, descriptor));

    s->activeRenderPass = renderPassHandle;

    return renderPassHandle;
}

void Graphics::SetGraphicsPipeline(GraphicsPipelineHandle graphicsPipeline)
{
    BX_ASSERT(s->activeRenderPass, "No render pass active.");
    BX_ENSURE(graphicsPipeline);

    s->boundGraphicsPipeline = graphicsPipeline;

    auto& pipelineIter = s->graphicsPipelines.find(graphicsPipeline);
    BX_ENSURE(pipelineIter != s->graphicsPipelines.end());
    auto& pipeline = pipelineIter->second;

    glUseProgram(pipeline.GetShaderProgramHandle());
    glBindVertexArray(pipeline.GetVaoHandle());

    auto& info = GetGraphicsPipelineCreateInfo(graphicsPipeline);
    
    if (info.depthFormat.IsSome())
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void Graphics::SetVertexBuffer(u32 slot, const BufferSlice& bufferSlice)
{
    BX_ASSERT(s->activeRenderPass, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ENSURE(bufferSlice.buffer);

    auto& bufferIter = s->buffers.find(bufferSlice.buffer);
    BX_ENSURE(bufferIter != s->buffers.end());

    auto& pipelineCreateInfo = GetGraphicsPipelineCreateInfo(s->boundGraphicsPipeline);
    u32 stride = pipelineCreateInfo.vertexBuffers[slot].stride;

    glBindVertexBuffer(slot, bufferIter->second, 0, stride);
}

void Graphics::SetIndexBuffer(const BufferSlice& bufferSlice, IndexFormat format)
{
    BX_ASSERT(s->activeRenderPass, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ENSURE(bufferSlice.buffer);

    auto& bufferIter = s->buffers.find(bufferSlice.buffer);
    BX_ENSURE(bufferIter != s->buffers.end());

    s->boundIndexFormat = Optional<IndexFormat>::Some(format);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIter->second);
}

void Graphics::SetBindGroup(u32 index, BindGroupHandle bindGroup)
{
    BX_ASSERT(s->activeRenderPass || s->activeComputePass, "No render pass or compute pass active.");
    BX_ENSURE(bindGroup);

    auto& bindGroupCreateInfoIter = s_createInfoCache->bindGroupCreateInfos.find(bindGroup);
    BX_ENSURE(bindGroupCreateInfoIter != s_createInfoCache->bindGroupCreateInfos.end());
    BindGroupCreateInfo& createInfo = bindGroupCreateInfoIter->second;

    u32 layoutIndex = GetBindGroupLayoutBindGroup(createInfo.layout);
    BX_ASSERT(layoutIndex == index, "Index {} must match with index {} in layout supplied by bind group create info.", index, layoutIndex);
    u64 rawPipeline = GetBindGroupLayoutPipeline(createInfo.layout);
    PipelineLayoutDescriptor layout;
    if (IsBindGroupLayoutGraphics(createInfo.layout))
    {
        auto& createInfoIter = s_createInfoCache->graphicsPipelineCreateInfos.find(GraphicsPipelineHandle{rawPipeline});
        BX_ENSURE(createInfoIter != s_createInfoCache->graphicsPipelineCreateInfos.end());
        layout = createInfoIter->second.layout;
    }
    else
    {
        auto& createInfoIter = s_createInfoCache->computePipelineCreateInfos.find(ComputePipelineHandle{ rawPipeline });
        BX_ENSURE(createInfoIter != s_createInfoCache->computePipelineCreateInfos.end());
        layout = createInfoIter->second.layout;
    }

    // TODO: optimize?
    OptionalView<BindGroupLayoutDescriptor> groupLayout = OptionalView<BindGroupLayoutDescriptor>::None();
    for (auto& group : layout.bindGroupLayouts)
    {
        if (group.group == index)
        {
            groupLayout = OptionalView<BindGroupLayoutDescriptor>::Some(&group);
            break;
        }
    }
    BX_ASSERT(groupLayout.IsSome(), "Group {} not found in layout.", index);

    // TODO: FAKE BIND GROUPS WITH OFFSETS!! (needs some macros in-shader)

    for (auto& entry : createInfo.entries)
    {
        const BindingResource& resource = entry.resource;

        // TODO: optimize?
        OptionalView<BindGroupLayoutEntry> groupLayoutEntry = OptionalView<BindGroupLayoutEntry>::None();
        for (auto& layoutEntry : groupLayout.Unwrap().entries)
        {
            if (layoutEntry.binding == entry.binding)
            {
                groupLayoutEntry = OptionalView<BindGroupLayoutEntry>::Some(&layoutEntry);
                break;
            }
        }
        BX_ASSERT(groupLayoutEntry.IsSome(), "Group {} binding {} not found in layout.", index, entry.binding);

        switch (resource.type)
        {
        case BindingResourceType::BUFFER:
        {
            if (groupLayoutEntry.Unwrap().type.type == BindingType::UNIFORM_BUFFER)
            {
                auto& bufferIter = s->buffers.find(resource.buffer.buffer);
                BX_ENSURE(bufferIter != s->buffers.end());

                glBindBufferBase(GL_UNIFORM_BUFFER, entry.binding, bufferIter->second);
            }
            else if (groupLayoutEntry.Unwrap().type.type == BindingType::STORAGE_BUFFER)
            {
                BX_FAIL("TODO");
            }
            else
            {
                BX_FAIL("Unexpected binding resource type at group {} binding {}.", index, entry.binding);
            }
            break;
        }
        case BindingResourceType::BUFFER_ARRAY:
        {
            BX_FAIL("TODO");
            break;
        }
        case BindingResourceType::TEXTURE_VIEW:
        {
            auto& textureViewIter = s->textureViews.find(resource.textureView);
            BX_ENSURE(textureViewIter != s->textureViews.end());

            if (groupLayoutEntry.Unwrap().type.type == BindingType::TEXTURE)
            {
                glActiveTexture(GL_TEXTURE0 + entry.binding);
                glBindTexture(GL_TEXTURE_2D, textureViewIter->second.texture);
            }
            else if (groupLayoutEntry.Unwrap().type.type == BindingType::STORAGE_TEXTURE)
            {
                TextureFormat format = GetTextureCreateInfo(textureViewIter->second.handle).format;
                BX_ASSERT(!IsTextureFormatSrgb(format), "Storage texture format cannot be srgb.");

                GLenum internalFormat = TextureFormatToGlInternalFormat(format);
                GLenum access = StorageTextureAccessToGl(groupLayoutEntry.Unwrap().type.storageTexture.access);

                glBindImageTexture(entry.binding, textureViewIter->second.texture, 0, GL_FALSE, 0, access, internalFormat);
            }
            else
            {
                BX_FAIL("Unexpected binding resource type at group {} binding {}.", index, entry.binding);
            }
            break;
        }
        }
    }
}

void Graphics::Draw(u32 vertexCount, u32 firstVertex, u32 instanceCount)
{
    BX_ASSERT(s->activeRenderPass, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ASSERT(instanceCount > 0, "Instance count must be larger than 0.");

    auto& info = GetGraphicsPipelineCreateInfo(s->boundGraphicsPipeline);

    if (instanceCount == 1)
        glDrawArrays(PrimitiveTopologyToGl(info.topology), firstVertex, vertexCount);
    else
        glDrawArraysInstanced(PrimitiveTopologyToGl(info.topology), firstVertex, vertexCount, instanceCount);
}

void Graphics::DrawIndexed(u32 indexCount, u32 instanceCount)
{
    BX_ASSERT(s->activeRenderPass, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ASSERT(s->boundIndexFormat.IsSome(), "No index buffer bound.");
    BX_ASSERT(instanceCount > 0, "Instance count must be larger than 0.");

    auto& info = GetGraphicsPipelineCreateInfo(s->boundGraphicsPipeline);
    GLenum indexType = IndexFormatToGl(s->boundIndexFormat.Unwrap());
    GLenum topology = PrimitiveTopologyToGl(info.topology);

    if (instanceCount == 1)
        glDrawElements(topology, indexCount, indexType, nullptr);
    else
        glDrawElementsInstanced(topology, indexCount, indexType, nullptr, instanceCount);
}

void Graphics::EndRenderPass(RenderPassHandle& renderPass)
{
    BX_ASSERT(s->activeRenderPass, "No render pass active.");
    BX_ENSURE(renderPass);

    const RenderPassDescriptor& descriptor = Graphics::GetRenderPassDescriptor(renderPass);
    for (u32 i = 0; i < descriptor.colorAttachments.size(); i++)
    {
        glNamedFramebufferTexture(s->framebuffer, GL_COLOR_ATTACHMENT0 + i, 0, 0);
    }
    if (descriptor.depthStencilAttachment.IsSome())
    {
        glNamedFramebufferTexture(s->framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
    }
    
    s->activeRenderPass = RenderPassHandle::null;
    s->renderPassHandlePool.Destroy(renderPass);
    s_createInfoCache->renderPassCreateInfos.erase(renderPass);

    s->boundGraphicsPipeline = GraphicsPipelineHandle::null;
}

ComputePassHandle Graphics::BeginComputePass(const ComputePassDescriptor& descriptor)
{
    BX_ASSERT(!s->activeComputePass, "Compute pass already active.");

    ComputePassHandle computePassHandle = s->computePassHandlePool.Create();
    //s_createInfoCache->renderPass.insert(std::make_pair(renderPass, descriptor));

    s->activeComputePass = computePassHandle;

    return computePassHandle;
}

void Graphics::SetComputePipeline(ComputePipelineHandle computePipeline)
{
    BX_ASSERT(s->activeComputePass, "No compute pass active.");
    BX_ENSURE(computePipeline);

    s->boundComputePipeline = computePipeline;

    auto& pipelineIter = s->computePipelines.find(computePipeline);
    BX_ENSURE(pipelineIter != s->computePipelines.end());
    auto& pipeline = pipelineIter->second;

    glUseProgram(pipeline.GetHandle());
}

void Graphics::DispatchWorkgroups(u32 x, u32 y, u32 z)
{
    BX_ASSERT(s->activeComputePass, "No compute pass active.");
    BX_ASSERT(s->boundComputePipeline, "No compute pipeline bound.");

    glDispatchCompute(x, y, z);
}

void Graphics::EndComputePass(ComputePassHandle& computePass)
{
    BX_ASSERT(s->activeComputePass, "No compute pass active.");
    BX_ENSURE(computePass);

    s->activeComputePass = ComputePassHandle::null;
    s->computePassHandlePool.Destroy(computePass);

    s->boundComputePipeline = ComputePipelineHandle::null;
}

void Graphics::WriteBuffer(BufferHandle buffer, u64 offset, const void* data)
{
    BX_ENSURE(buffer && data);
    BX_ASSERT(offset == 0, "Offset must be 0 for now.");

    auto& bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());

    glNamedBufferData(bufferIter->second, GetBufferCreateInfo(buffer).size, data, GL_DYNAMIC_DRAW);
}

void Graphics::WriteBuffer(BufferHandle buffer, u64 offset, const void* data, SizeType size)
{
    BX_ENSURE(buffer && data);
    BX_ASSERT(offset == 0, "Offset must be 0 for now.");

    auto& bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());

    glNamedBufferData(bufferIter->second, size, data, GL_DYNAMIC_DRAW);
}

void Graphics::WriteTexture(TextureHandle texture, const void* data, const Extend3D& offset, const Extend3D& size)
{
    BX_ENSURE(texture && data);

    auto& textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());

    auto createInfo = GetTextureCreateInfo(texture);

    if (createInfo.dimension == TextureDimension::D2 || createInfo.size.depthOrArrayLayers == 1)
    {
        glTextureSubImage2D(
            textureIter->second,
            0,
            offset.width,
            offset.height,
            size.width,
            size.height,
            TextureFormatToGlFormat(createInfo.format),
            TextureFormatToGlType(createInfo.format),
            data
        );
    }
    else
    {
        BX_FAIL("TODO");
    }
}

void Graphics::ReadTexture(TextureHandle texture, void* data, const Extend3D& offset, const Extend3D& size)
{
    BX_ENSURE(texture);

    auto& textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());

    auto createInfo = GetTextureCreateInfo(texture);

    glGetTextureSubImage(
        textureIter->second,
        0,
        offset.width, offset.height, offset.depthOrArrayLayers,
        size.width, size.height, size.depthOrArrayLayers,
        TextureFormatToGlFormat(createInfo.format),
        TextureFormatToGlType(createInfo.format),
        SizeOfTextureFormat(createInfo.format),
        data
    );
}

GLuint GraphicsOpenGL::GetRawBufferHandle(BufferHandle buffer)
{
    BX_ENSURE(buffer);

    auto& bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());

    return bufferIter->second;
}

GLuint GraphicsOpenGL::GetRawTextureHandle(TextureHandle texture)
{
    BX_ENSURE(texture);

    auto& textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());

    return textureIter->second;
}

// TODO: remove!
void Graphics::DebugDraw(const Mat4& viewProj, const DebugDrawAttribs& attribs, const List<DebugVertex>& vertices)
{
}