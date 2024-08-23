#include "bx/engine/modules/graphics/backend/graphics_vulkan.hpp"

#include "bx/engine/modules/graphics/type_validation.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/engine/core/profiler.hpp"
#include "bx/engine/core/memory.hpp"
#include "bx/engine/core/macros.hpp"
#include "bx/engine/containers/array.hpp"
#include "bx/engine/containers/list.hpp"

#include "bx/engine/modules/window.hpp"
#include "bx/engine/modules/imgui.hpp"

#include "bx/engine/modules/graphics/backend/vulkan/acceleration_structure.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/resource_state_tracker.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/conversion.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/compute_pipeline.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/buffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/instance.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/physical_device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/device.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/fence.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/semaphore.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/render_pass.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/image.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/sampler.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/swapchain.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/framebuffer.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/cmd_queue.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/cmd_list.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/descriptor_pool.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/descriptor_set_layout.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/shader.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/graphics_pipeline.hpp"
#include "bx/engine/modules/graphics/backend/vulkan/rect2d.hpp"
using namespace Vk;

#ifdef BX_WINDOW_GLFW_BACKEND
#include "bx/engine/modules/window/backend/window_glfw.hpp"
#endif

#include <utility>

constexpr bool ENABLE_VALIDATION =
#ifdef _DEBUG
true;
#else
false;
#endif

struct TextureView
{
    TextureHandle handle;
    std::shared_ptr<Image> texture;
};

struct State : NoCopy
{
    ~State()
    {
        device->WaitIdle();
    }

    GraphicsCapabilities graphicsCapabilities;

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
    HandlePool<BlasApi> blasHandlePool;
    HandlePool<TlasApi> tlasHandlePool;

    HashMap<BufferHandle, std::shared_ptr<Buffer>> buffers;
    HashMap<TextureHandle, std::shared_ptr<Image>> textures;
    HashMap<TextureViewHandle, TextureView> textureViews;
    HashMap<ShaderHandle, std::shared_ptr<Shader>> shaders;
    HashMap<GraphicsPipelineHandle, HashMap<RenderPassInfo, std::shared_ptr<GraphicsPipeline>>> graphicsPipelines;
    HashMap<GraphicsPipelineHandle, const List<std::shared_ptr<DescriptorSetLayout>>> graphicsPipelineLayouts;
    HashMap<ComputePipelineHandle, std::shared_ptr<ComputePipeline>> computePipelines;
    HashMap<ComputePipelineHandle, const List<std::shared_ptr<DescriptorSetLayout>>> computePipelineLayouts;
    HashMap<BindGroupHandle, std::shared_ptr<DescriptorSet>> bindGroups;
    HashMap<BlasHandle, std::shared_ptr<Blas>> blases;
    HashMap<TlasHandle, std::shared_ptr<Tlas>> tlases;

    std::shared_ptr<Framebuffer> renderPassFramebuffer = nullptr;
    std::shared_ptr<RenderPass> activeRenderPass = nullptr;
    b8 isRenderPassBound = false;
    Optional<RenderPassInfo> activeRenderPassInfo = Optional<RenderPassInfo>::None();

    RenderPassHandle activeRenderPassHandle = RenderPassHandle::null;
    ComputePassHandle activeComputePass = ComputePassHandle::null;
    GraphicsPipelineHandle boundGraphicsPipeline = GraphicsPipelineHandle::null;
    ComputePipelineHandle boundComputePipeline = ComputePipelineHandle::null;
    Optional<IndexFormat> boundIndexFormat = Optional<IndexFormat>::None();

    BufferHandle emptyBuffer = BufferHandle::null;
    TextureHandle emptyTexture = TextureHandle::null;

    Array<TextureHandle, Swapchain::MAX_FRAMES_IN_FLIGHT> swapchainColorTargets = {};
    Array<TextureViewHandle, Swapchain::MAX_FRAMES_IN_FLIGHT> swapchainColorTargetViews = {};

    std::shared_ptr<Instance> instance;
    std::unique_ptr<PhysicalDevice> physicalDevice;
    std::shared_ptr<Device> device;
    std::unique_ptr<CmdQueue> cmdQueue;
    std::shared_ptr<DescriptorPool> descriptorPool;
    std::unique_ptr<Swapchain> swapchain;
    std::shared_ptr<Sampler> sampler;

    std::shared_ptr<Fence> presentFence;
    std::shared_ptr<CmdList> cmdList;
    std::shared_ptr<CmdList> uploadCmdList;
};
static std::unique_ptr<State> s;

struct RenderPassCache : public LazyInitMap<RenderPassCache, std::shared_ptr<RenderPass>, RenderPassInfo>
{
    RenderPassCache(const RenderPassInfo& args)
    {
        data = std::shared_ptr<RenderPass>(new RenderPass("Render Pass", s->device, args));
    }
};

template<>
HashMap<RenderPassInfo, std::unique_ptr<RenderPassCache>> LazyInitMap<RenderPassCache, std::shared_ptr<RenderPass>, RenderPassInfo>::cache = {};

void BuildSwapchain(HashMap<TextureHandle, TextureCreateInfo>& textureCreateInfos)
{
    i32 width, height;
    Window::GetSize(&width, &height);

    s->swapchain.reset();
    s->swapchain = std::unique_ptr<Swapchain>(new Swapchain(static_cast<u32>(width), static_cast<u32>(height), *s->instance, s->device, *s->physicalDevice));

    for (u32 i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (s->swapchainColorTargets[i])
        {
            Graphics::DestroyTexture(s->swapchainColorTargets[i]);
        }

        if (s->swapchainColorTargetViews[i])
        {
            Graphics::DestroyTextureView(s->swapchainColorTargetViews[i]);
        }
    }

    for (u32 i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        TextureHandle textureHandle = s->textureHandlePool.Create();
        TextureViewHandle textureViewHandle = s->textureViewHandlePool.Create();
        textureCreateInfos.insert(std::make_pair(textureHandle, s->swapchain->GetImageCreateInfo()));

        std::shared_ptr<Image> image = s->swapchain->GetImage(i);
        TextureView textureView{};
        textureView.handle = textureHandle;
        textureView.texture = image;

        s->textures.emplace(textureHandle, image);
        s->textureViews.emplace(textureViewHandle, textureView);
        s->swapchainColorTargets[i] = textureHandle;
        s->swapchainColorTargetViews[i] = textureViewHandle;
    }
}

bool Graphics::Initialize()
{
    s_createInfoCache = std::unique_ptr<CreateInfoCache>(new CreateInfoCache());
    s = std::unique_ptr<State>(new State());

#ifdef BX_WINDOW_GLFW_BACKEND
    GLFWwindow* glfwWindow = WindowGLFW::GetWindowPtr();

    s->instance = std::shared_ptr<Instance>(new Instance((void*)glfwWindow, ENABLE_VALIDATION));
#else

    BX_LOGE("Window backend not supported!");
    return false;
#endif
    
    s->physicalDevice = std::unique_ptr<PhysicalDevice>(new PhysicalDevice(*s->instance));
    s->device = std::shared_ptr<Device>(new Device(s->instance, *s->physicalDevice, ENABLE_VALIDATION));
    s->cmdQueue = std::unique_ptr<CmdQueue>(new CmdQueue(s->device, *s->physicalDevice, QueueType::GRAPHICS));
    s->descriptorPool = std::shared_ptr<DescriptorPool>(new DescriptorPool(s->device));
    s->sampler = std::shared_ptr<Sampler>(new Sampler("Sampler", s->device, *s->physicalDevice,
        SamplerInfo{}));

    BufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.name = "Empty Buffer";
    bufferCreateInfo.size = 1;
    bufferCreateInfo.usageFlags = BufferUsageFlags::COPY_SRC | BufferUsageFlags::INDEX | BufferUsageFlags::VERTEX | BufferUsageFlags::UNIFORM | BufferUsageFlags::STORAGE;
    s->emptyBuffer = Graphics::CreateBuffer(bufferCreateInfo);

    TextureCreateInfo textureCreateInfo{};
    textureCreateInfo.name = "Empty Texture";
    textureCreateInfo.size = Extend3D(1, 1, 1);
    textureCreateInfo.usageFlags = TextureUsageFlags::COPY_SRC | TextureUsageFlags::TEXTURE_BINDING | TextureUsageFlags::STORAGE_BINDING;

    BuildSwapchain(s_createInfoCache->textureCreateInfos);

    GraphicsCapabilities capabilities{};
    capabilities.raytracing = s->physicalDevice->RayTracingSuitable();
    s->graphicsCapabilities = capabilities;

    return true;
}

void Graphics::Shutdown()
{
    s.reset();
}

void Graphics::Reload()
{
    // TODO: No implementation
}

void Graphics::NewFrame()
{
    s->cmdQueue->ProcessCmdLists(true);

    if (Window::IsActive())
    {
        if (Window::WasResized())
        {
            s->device->WaitIdle();

            BuildSwapchain(s_createInfoCache->textureCreateInfos);
        }

        s->presentFence = s->swapchain->NextImage();
    }

    // All cmds of the entire frame will be recorded into a single cmd list
    // This is because we designed the graphics module api to act like it's immediate
    s->cmdList = s->cmdQueue->GetCmdList("Main Cmd List");
}

void Graphics::EndFrame()
{
    s->cmdQueue->SubmitCmdList(s->uploadCmdList, nullptr, {}, {}, {});
    s->uploadCmdList.reset();

    if (Window::IsActive())
    {
        size_t currentFrame = static_cast<size_t>(s->swapchain->GetCurrentFrameIdx());

#ifdef BX_EDITOR_BUILD
        Rect2D swapchainExtent = s->swapchain->Extent();

        s->cmdList->TransitionImageLayout(s->swapchain->GetImage(currentFrame),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        s->cmdList->BeginRenderPass(s->swapchain->GetRenderPass(),
            s->swapchain->GetCurrentFramebuffer(),
            Color(0.1f, 0.1f, 0.1f, 1.0f));
        s->cmdList->SetScissor(swapchainExtent);
        s->cmdList->SetViewport(swapchainExtent);
        ImGuiImpl::EndFrame();
        s->cmdList->EndRenderPass();
        ResourceStateTracker::ApplyImplicitImageTransition(*s->swapchain->GetImage(currentFrame),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#endif

        s->cmdList->TransitionImageLayout(s->swapchain->GetImage(currentFrame),
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

        // Execute all rendering cmds when the image is available
        List<Semaphore*> waitSemaphores{ &s->swapchain->GetImageAvailableSemaphore() };
        List<VkPipelineStageFlags> presentWaitStages{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        List<Semaphore*> presentSignalSemaphores{
            &s->swapchain->GetRenderFinishedSemaphore() };
        s->cmdQueue->SubmitCmdList(s->cmdList, s->presentFence, waitSemaphores, presentWaitStages,
            presentSignalSemaphores);
        
        // Present when rendering is finished, indicated by the `presentSignalSemaphores`
        s->swapchain->Present(*s->cmdQueue, *s->presentFence, presentSignalSemaphores);
        ResourceStateTracker::ApplyImplicitImageTransition(*s->swapchain->GetImage(currentFrame),
            VK_IMAGE_LAYOUT_UNDEFINED);
    }
    else
    {
        ImGuiImpl::EndFrame();
    }

    s->cmdList.reset();
}

const GraphicsCapabilities Graphics::GetCapabilities()
{
    return s->graphicsCapabilities;
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
    return s->swapchainColorTargets[s->swapchain->GetCurrentFrameIdx()];
}

TextureViewHandle Graphics::GetSwapchainColorTargetView()
{
    return s->swapchainColorTargetViews[s->swapchain->GetCurrentFrameIdx()];
}

TextureHandle Graphics::CreateTexture(const TextureCreateInfo& _createInfo)
{
    TextureCreateInfo createInfo = _createInfo;
    createInfo.usageFlags = createInfo.usageFlags | TextureUsageFlags::COPY_DST;

    BX_ENSURE(ValidateTextureCreateInfo(createInfo));

    TextureHandle textureHandle = s->textureHandlePool.Create();
    s_createInfoCache->textureCreateInfos.insert(std::make_pair(textureHandle, createInfo.WithoutData()));

    b8 isDepth = IsTextureFormatDepth(createInfo.format);
    VkImageUsageFlags usage = TextureUsageFlagsToVk(createInfo.usageFlags, isDepth);
    VkFormat format = TextureFormatToVk(createInfo.format);
    VkImageType type = TextureDimensionToVk(createInfo.dimension);
    
    u32 depth = (createInfo.dimension == TextureDimension::D3) ? createInfo.size.depthOrArrayLayers : 1;
    u32 arrayLayers = (createInfo.dimension != TextureDimension::D3) ? createInfo.size.depthOrArrayLayers : 1;

    std::shared_ptr<Image> image(new Image(createInfo.name, s->device, *s->physicalDevice,
        createInfo.size.width, createInfo.size.height, createInfo.mipLevelCount, usage, format, arrayLayers, type, depth));
    s->textures.emplace(textureHandle, image);

    if (createInfo.data)
    {
        WriteTexture(textureHandle, createInfo.data, Extend3D(0, 0, 0), createInfo.size);
    }

    return textureHandle;
}

void Graphics::DestroyTexture(TextureHandle& texture)
{
    BX_ENSURE(texture);

    auto textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());

    s->textures.erase(texture);
    s_createInfoCache->textureCreateInfos.erase(texture);
    s->textureHandlePool.Destroy(texture);
}

TextureViewHandle Graphics::CreateTextureView(TextureHandle texture)
{
    BX_ENSURE(texture);

    TextureViewHandle textureViewHandle = s->textureViewHandlePool.Create();

    auto textureIter = s->textures.find(texture);
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
    BX_FAIL("TODO");
}

BufferHandle Graphics::CreateBuffer(const BufferCreateInfo& _createInfo)
{
    BufferCreateInfo createInfo = _createInfo;

    b8 isMappable = IsBufferUsageMappable(createInfo.usageFlags);
    if (!isMappable)
        createInfo.usageFlags = createInfo.usageFlags | BufferUsageFlags::COPY_DST;

    BX_ENSURE(ValidateBufferCreateInfo(createInfo));

    BufferHandle bufferHandle = s->bufferHandlePool.Create();
    s_createInfoCache->bufferCreateInfos.insert(std::make_pair(bufferHandle, createInfo.WithoutData()));

    VkBufferUsageFlags usage = BufferUsageFlagsToVk(createInfo.usageFlags);
    BufferLocation location = isMappable ? BufferLocation::CPU_TO_GPU : BufferLocation::GPU_ONLY;

    std::shared_ptr<Buffer> buffer(new Buffer(createInfo.name, s->device, *s->physicalDevice, usage, createInfo.size, location));
    s->buffers.emplace(bufferHandle, buffer);

    if (createInfo.data)
    {
        if (!isMappable)
        {
            WriteBuffer(bufferHandle, 0, createInfo.data);
        }
        else
        {
            void* bufferData = buffer->Map();
            memcpy(bufferData, createInfo.data, createInfo.size);
            buffer->Unmap();
        }
    }

    return bufferHandle;
}

void Graphics::DestroyBuffer(BufferHandle& buffer)
{
    BX_ENSURE(buffer);

    auto bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());

    s->buffers.erase(buffer);
    s_createInfoCache->bufferCreateInfos.erase(buffer);
    s->bufferHandlePool.Destroy(buffer);
}

ShaderHandle Graphics::CreateShader(const ShaderCreateInfo& createInfo)
{
    BX_ENSURE(ValidateShaderCreateInfo(createInfo));

    ShaderHandle shaderHandle = s->shaderHandlePool.Create();
    s_createInfoCache->shaderCreateInfos.insert(std::make_pair(shaderHandle, createInfo));

    String meta = String(R""""(
    #version 460
    
    #ifndef VULKAN
    #define VULKAN
    #endif // VULKAN
    )"""");

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

    VkShaderStageFlagBits stage = ShaderTypeToVk(createInfo.shaderType);

    std::shared_ptr<Shader> shader(new Shader(createInfo.name, s->device, stage, meta + createInfo.src));
    s->shaders.emplace(shaderHandle, shader);

    return shaderHandle;
}

void Graphics::DestroyShader(ShaderHandle& shader)
{
    BX_ENSURE(shader);

    // Graphics pipelines are created on demand during SetGraphicsPipeline, immediately destroying shaders is therefore not valid
    /*auto shaderIter = s->shaders.find(shader);
    BX_ENSURE(shaderIter != s->shaders.end());

    s->shaders.erase(shader);
    s_createInfoCache->shaderCreateInfos.erase(shader);
    s->shaderHandlePool.Destroy(shader);*/
}

GraphicsPipelineHandle Graphics::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
{
    BX_ENSURE(ValidateGraphicsPipelineCreateInfo(createInfo));

    GraphicsPipelineHandle graphicsPipelineHandle = s->graphicsPipelineHandlePool.Create();
    s_createInfoCache->graphicsPipelineCreateInfos.insert(std::make_pair(graphicsPipelineHandle, createInfo));

    s->graphicsPipelines.insert(std::make_pair(graphicsPipelineHandle, HashMap<RenderPassInfo, std::shared_ptr<GraphicsPipeline>>{}));

    List<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayouts{};
    for (auto& layout : createInfo.layout.bindGroupLayouts)
    {
        List<VkDescriptorSetLayoutBinding> bindings{};
        for (auto& entry : layout.entries)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = entry.binding;
            binding.descriptorCount = entry.count.IsSome() ? entry.count.Unwrap() : 1;
            binding.descriptorType = BindingTypeToVk(entry.type.type);
            binding.stageFlags = ShaderStageFlagsToVk(entry.visibility);
            bindings.push_back(binding);
        }

        std::shared_ptr<DescriptorSetLayout> descriptorSetLayout(new DescriptorSetLayout(Log::Format("{} layout", createInfo.name.c_str()), s->device, bindings));
        descriptorSetLayouts.push_back(descriptorSetLayout);
    }

    s->graphicsPipelineLayouts.emplace(std::piecewise_construct,
        std::forward_as_tuple(graphicsPipelineHandle),
        std::forward_as_tuple(std::move(descriptorSetLayouts)));

    return graphicsPipelineHandle;
}

void Graphics::DestroyGraphicsPipeline(GraphicsPipelineHandle& graphicsPipeline)
{
    BX_ENSURE(graphicsPipeline);

    s->graphicsPipelines.erase(graphicsPipeline);
    s->graphicsPipelineLayouts.erase(graphicsPipeline);
    s_createInfoCache->graphicsPipelineCreateInfos.erase(graphicsPipeline);
    s->graphicsPipelineHandlePool.Destroy(graphicsPipeline);
}

ComputePipelineHandle Graphics::CreateComputePipeline(const ComputePipelineCreateInfo& createInfo)
{
    BX_ENSURE(ValidateComputePipelineCreateInfo(createInfo));

    ComputePipelineHandle computePipelineHandle = s->computePipelineHandlePool.Create();
    s_createInfoCache->computePipelineCreateInfos.insert(std::make_pair(computePipelineHandle, createInfo));

    List<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayouts{};
    for (auto& layout : createInfo.layout.bindGroupLayouts)
    {
        List<VkDescriptorSetLayoutBinding> bindings{};
        for (auto& entry : layout.entries)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = entry.binding;
            binding.descriptorCount = entry.count.IsSome() ? entry.count.Unwrap() : 1;
            binding.descriptorType = BindingTypeToVk(entry.type.type);
            binding.stageFlags = ShaderStageFlagsToVk(entry.visibility);
            bindings.push_back(binding);
        }

        std::shared_ptr<DescriptorSetLayout> descriptorSetLayout(new DescriptorSetLayout(Log::Format("{} layout", createInfo.name.c_str()), s->device, bindings));
        descriptorSetLayouts.push_back(descriptorSetLayout);
    }

    auto shaderIter = s->shaders.find(createInfo.shader);
    BX_ENSURE(shaderIter != s->shaders.end());

    std::shared_ptr<ComputePipeline> computePipeline(new ComputePipeline(
        s->device,
        shaderIter->second,
        descriptorSetLayouts));
    s->computePipelines.insert(std::make_pair(computePipelineHandle, computePipeline));

    s->computePipelineLayouts.emplace(std::piecewise_construct,
        std::forward_as_tuple(computePipelineHandle),
        std::forward_as_tuple(std::move(descriptorSetLayouts)));

    return computePipelineHandle;
}

void Graphics::DestroyComputePipeline(ComputePipelineHandle& computePipeline)
{
    BX_ENSURE(computePipeline);

    s->computePipelines.erase(computePipeline);
    s->computePipelineLayouts.erase(computePipeline);
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

    u32 layoutIndex = GetBindGroupLayoutBindGroup(createInfo.layout);
    u64 rawPipeline = GetBindGroupLayoutPipeline(createInfo.layout);
    std::shared_ptr<DescriptorSetLayout> layout;
    if (IsBindGroupLayoutGraphics(createInfo.layout))
    {
        auto layoutIter = s->graphicsPipelineLayouts.find(GraphicsPipelineHandle{ rawPipeline });
        BX_ENSURE(layoutIter != s->graphicsPipelineLayouts.end());
        layout = layoutIter->second[layoutIndex];
    }
    else
    {
        auto layoutIter = s->computePipelineLayouts.find(ComputePipelineHandle{ rawPipeline });
        BX_ENSURE(layoutIter != s->computePipelineLayouts.end());
        layout = layoutIter->second[layoutIndex];
    }

    std::shared_ptr<DescriptorSet> descriptorSet(new DescriptorSet(createInfo.name, s->device, s->descriptorPool, layout));

    for (auto& entry : createInfo.entries)
    {
        switch (entry.resource.type)
        {
        case BindingResourceType::BUFFER:
        {
            auto bufferIter = s->buffers.find(entry.resource.buffer.buffer);
            BX_ENSURE(bufferIter != s->buffers.end());

            VkDescriptorType type = layout->GetDescriptorType(entry.binding);
            descriptorSet->SetBuffer(entry.binding, type, bufferIter->second);
            break;
        }
        case BindingResourceType::TEXTURE_VIEW:
        {
            auto textureViewIter = s->textureViews.find(entry.resource.textureView);
            BX_ENSURE(textureViewIter != s->textureViews.end());

            VkDescriptorType type = layout->GetDescriptorType(entry.binding);
            std::shared_ptr<Sampler> sampler = type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ? s->sampler : nullptr;
            descriptorSet->SetImage(entry.binding, type, textureViewIter->second.texture, sampler);
            break;
        }
        case BindingResourceType::ACCELERATION_STRUCTURE:
        {
            auto tlasIter = s->tlases.find(entry.resource.accelerationStructure);
            BX_ENSURE(tlasIter != s->tlases.end());

            VkDescriptorType type = layout->GetDescriptorType(entry.binding);
            descriptorSet->SetAccelerationStructure(entry.binding, tlasIter->second);
            break;
        }
        default:
            BX_FAIL("TODO");
        }
    }

    s->bindGroups.insert(std::make_pair(bindGroupHandle, descriptorSet));

    return bindGroupHandle;
}

void Graphics::DestroyBindGroup(BindGroupHandle& bindGroup)
{
    BX_ENSURE(bindGroup);

    s->bindGroups.erase(bindGroup);
    s_createInfoCache->bindGroupCreateInfos.erase(bindGroup);
    s->bindGroupHandlePool.Destroy(bindGroup);
}

const BlasHandle Graphics::CreateBlas(const BlasCreateInfo& createInfo)
{
    BX_ASSERT(s->graphicsCapabilities.raytracing, "Raytracing is not supported, please check `GraphicsCapabilities` first.");
    BX_ENSURE(ValidateBlasCreateInfo(createInfo));

    BlasHandle blasHandle = s->blasHandlePool.Create();
    s_createInfoCache->blasCreateInfos.insert(std::make_pair(blasHandle, createInfo));

    auto vertexBufferIter = s->buffers.find(createInfo.vertexBuffer.buffer);
    auto indexBufferIter = s->buffers.find(createInfo.indexBuffer.buffer);
    BX_ENSURE(vertexBufferIter != s->buffers.end());
    BX_ENSURE(indexBufferIter != s->buffers.end());
    
    u32 indexStride = SizeOfIndexFormat(createInfo.indexFormat);
    u32 vertexCount = (createInfo.vertexBuffer.size.IsSome() ? createInfo.vertexBuffer.size.Unwrap() : vertexBufferIter->second->Size()) / createInfo.vertexStride;
    u32 indexCount = (createInfo.indexBuffer.size.IsSome() ? createInfo.indexBuffer.size.Unwrap() : indexBufferIter->second->Size()) / indexStride;

    VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
    triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    triangles.vertexData.deviceAddress = vertexBufferIter->second->GetDeviceAddress();
    triangles.vertexStride = createInfo.vertexStride;
    triangles.maxVertex = vertexCount - 1;
    triangles.vertexFormat = VertexFormatToVk(createInfo.vertexFormat);
    triangles.indexData.deviceAddress = indexBufferIter->second->GetDeviceAddress();
    triangles.indexType = IndexFormatToVk(createInfo.indexFormat);

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.triangles = triangles;

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.firstVertex = createInfo.vertexBuffer.offset / createInfo.vertexStride;
    rangeInfo.primitiveCount = indexCount / 3;
    rangeInfo.primitiveOffset = createInfo.indexBuffer.offset / indexStride;
    rangeInfo.transformOffset = 0;

    VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

    u32 blasSize = Blas::RequiredSize(s->device, *s->physicalDevice, geometry, indexCount / 3, flags);
    std::shared_ptr<Blas> blas(new Blas(createInfo.name, s->device, *s->physicalDevice, blasSize));
    if (!s->uploadCmdList) s->uploadCmdList = s->cmdQueue->GetCmdList("Upload Cmd List");
    blas->Build(*s->uploadCmdList, geometry, rangeInfo, flags);

    s->blases.insert(std::make_pair(blasHandle, blas));

    return blasHandle;
}

void Graphics::DestroyBlas(BlasHandle& blas)
{
    BX_ASSERT(s->graphicsCapabilities.raytracing, "Raytracing is not supported, please check `GraphicsCapabilities` first.");

    s->blases.erase(blas);
    s_createInfoCache->blasCreateInfos.erase(blas);
    s->blasHandlePool.Destroy(blas);
}

const TlasHandle Graphics::CreateTlas(const TlasCreateInfo& createInfo)
{
    BX_ASSERT(s->graphicsCapabilities.raytracing, "Raytracing is not supported, please check `GraphicsCapabilities` first.");
    BX_ENSURE(ValidateTlasCreateInfo(createInfo));

    TlasHandle tlasHandle = s->tlasHandlePool.Create();
    s_createInfoCache->tlasCreateInfos.insert(std::make_pair(tlasHandle, createInfo));

    List<VkAccelerationStructureInstanceKHR> instances{};
    instances.reserve(createInfo.blasInstances.size());
    for (u32 i = 0; i < createInfo.blasInstances.size(); i++)
    {
        const BlasInstance& blasInstance = createInfo.blasInstances[i];

        auto blasIter = s->blases.find(blasInstance.blas);
        BX_ENSURE(blasIter != s->blases.end());

        VkAccelerationStructureInstanceKHR vkInstance{};
        Mat4 transform = blasInstance.transform.Transpose();
        memcpy(&vkInstance.transform, transform.data, sizeof(VkTransformMatrixKHR));
        vkInstance.instanceCustomIndex = blasInstance.instanceCustomIndex;
        vkInstance.mask = blasInstance.mask;
        vkInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR | VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR; // TODO: ??
        vkInstance.accelerationStructureReference = blasIter->second->GetBuffer()->GetDeviceAddress();

        instances.push_back(vkInstance);
    }

    u32 instancesSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    std::shared_ptr<Buffer> instancesBuffer(new Buffer("Tlas Instances Buffer", s->device, *s->physicalDevice,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT, instancesSize, BufferLocation::GPU_ONLY));

    std::shared_ptr<Buffer> stagingBuffer(new Buffer("Write Tlas Instances Staging Buffer", s->device,
        *s->physicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        static_cast<uint64_t>(instancesSize), BufferLocation::CPU_TO_GPU));

    void* bufferData = stagingBuffer->Map();
    memcpy(bufferData, instances.data(), instancesSize);
    stagingBuffer->Unmap();

    if (!s->uploadCmdList) s->uploadCmdList = s->cmdQueue->GetCmdList("Upload Cmd List");
    s->uploadCmdList->CopyBuffers(stagingBuffer, instancesBuffer);

    VkAccelerationStructureGeometryInstancesDataKHR instancesData{};
    instancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    instancesData.arrayOfPointers = false;
    instancesData.data.deviceAddress = instancesBuffer->GetDeviceAddress();

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.instances = instancesData;

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.firstVertex = 0;
    rangeInfo.primitiveCount = instances.size();
    rangeInfo.primitiveOffset = 0;
    rangeInfo.transformOffset = 0;

    VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

    u32 tlasSize = Tlas::RequiredSize(s->device, *s->physicalDevice, geometry, instances.size(), flags);
    std::shared_ptr<Tlas> tlas(new Tlas(createInfo.name, s->device, *s->physicalDevice, tlasSize));
    tlas->Build(*s->uploadCmdList, geometry, rangeInfo, flags);

    // TODO: avoid doing this loop twice
    for (u32 i = 0; i < createInfo.blasInstances.size(); i++)
    {
        const BlasInstance& blasInstance = createInfo.blasInstances[i];

        auto blasIter = s->blases.find(blasInstance.blas);
        BX_ENSURE(blasIter != s->blases.end());

        tlas->TrackBlas(blasIter->second);
    }

    s->tlases.insert(std::make_pair(tlasHandle, tlas));

    return tlasHandle;
}

void Graphics::DestroyTlas(TlasHandle& tlas)
{
    BX_ASSERT(s->graphicsCapabilities.raytracing, "Raytracing is not supported, please check `GraphicsCapabilities` first.");

    s->tlases.erase(tlas);
    s_createInfoCache->tlasCreateInfos.erase(tlas);
    s->tlasHandlePool.Destroy(tlas);
}

RenderPassHandle Graphics::BeginRenderPass(const RenderPassDescriptor& descriptor)
{
    BX_ASSERT(!s->activeRenderPassHandle, "Render pass already active.");
    BX_ASSERT(!s->activeComputePass, "Compute pass already active.");

    u32 width = 0;
    u32 height = 0;

    RenderPassInfo renderPassInfo{};
    FramebufferInfo framebufferInfo{};
    for (auto& colorAttachment : descriptor.colorAttachments)
    {
        auto textureViewIter = s->textureViews.find(colorAttachment.view);
        BX_ENSURE(textureViewIter != s->textureViews.end());
        auto& textureCreateInfo = GetTextureCreateInfo(textureViewIter->second.handle);

        framebufferInfo.images.push_back(textureViewIter->second.texture);
        s->cmdList->TransitionImageLayout(textureViewIter->second.texture,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        VkFormat format = TextureFormatToVk(textureCreateInfo.format);
        renderPassInfo.colorFormats.push_back(format);

        if (width == 0 && height == 0)
        {
            auto createInfo = GetTextureCreateInfo(textureViewIter->second.handle);
            width = createInfo.size.width;
            height = createInfo.size.height;
        }
    }
    if (descriptor.depthStencilAttachment.IsSome())
    {
        auto& depthAttachment = descriptor.depthStencilAttachment.Unwrap();

        auto textureViewIter = s->textureViews.find(depthAttachment.view);
        BX_ENSURE(textureViewIter != s->textureViews.end());
        auto& textureCreateInfo = GetTextureCreateInfo(textureViewIter->second.handle);

        framebufferInfo.images.push_back(textureViewIter->second.texture);
        s->cmdList->TransitionImageLayout(textureViewIter->second.texture,
            TextureFormatToVkImageLayout(textureCreateInfo.format),
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

        VkFormat format = TextureFormatToVk(textureCreateInfo.format);
        renderPassInfo.depthFormat = Optional<VkFormat>::Some(format);

        if (width == 0 && height == 0)
        {
            auto createInfo = GetTextureCreateInfo(textureViewIter->second.handle);
            width = createInfo.size.width;
            height = createInfo.size.height;
        }
    }

    std::shared_ptr<RenderPass> loadRenderPass = RenderPassCache::Get(renderPassInfo);

    RenderPassInfo noClearRenderPassInfo = renderPassInfo; // TODO: check if duplicate is required
    noClearRenderPassInfo.clear = false;
    std::shared_ptr<RenderPass> renderPass = RenderPassCache::Get(noClearRenderPassInfo);

    framebufferInfo.renderPass = renderPass;
    
    std::shared_ptr<Framebuffer> framebuffer(new Framebuffer("framebuffer", s->device, framebufferInfo));
    s->cmdList->SetViewport(Rect2D(width, height));
    s->cmdList->SetScissor(Rect2D(width, height));

    s->cmdList->BeginRenderPass(loadRenderPass, framebuffer, Color::Black());
    s->cmdList->EndRenderPass();

    RenderPassHandle renderPassHandle = s->renderPassHandlePool.Create();
    s_createInfoCache->renderPassCreateInfos.insert(std::make_pair(renderPassHandle, descriptor));

    s->activeRenderPassInfo = Optional<RenderPassInfo>::Some(renderPassInfo);
    s->renderPassFramebuffer = framebuffer;
    s->activeRenderPass = framebufferInfo.renderPass;
    s->activeRenderPassHandle = renderPassHandle;

    return renderPassHandle;
}

void Graphics::SetGraphicsPipeline(GraphicsPipelineHandle graphicsPipelineHandle)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ENSURE(graphicsPipelineHandle);

    s->boundGraphicsPipeline = graphicsPipelineHandle;

    auto& graphicsPipelineIter = s->graphicsPipelines.find(graphicsPipelineHandle);
    BX_ENSURE(graphicsPipelineIter != s->graphicsPipelines.end());
    auto& graphicsPipelineMap = graphicsPipelineIter->second;

    std::shared_ptr<GraphicsPipeline> graphicsPipeline;
    auto& graphicsPipelineMapIter = graphicsPipelineMap.find(s->activeRenderPassInfo.Unwrap());
    if (graphicsPipelineMapIter == graphicsPipelineMap.end())
    {
        auto& createInfo = GetGraphicsPipelineCreateInfo(graphicsPipelineHandle);

        auto& vertexShaderIter = s->shaders.find(createInfo.vertexShader);
        BX_ENSURE(vertexShaderIter != s->shaders.end());
        auto& fragmentShaderIter = s->shaders.find(createInfo.fragmentShader);
        BX_ENSURE(fragmentShaderIter != s->shaders.end());
        std::shared_ptr<Shader> vertexShader = vertexShaderIter->second;
        std::shared_ptr<Shader> fragmentShader = fragmentShaderIter->second;

        List<VkVertexInputBindingDescription> vertexBindingDescriptions{};
        for (u32 i = 0; i < createInfo.vertexBuffers.size(); i++)
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = i;
            bindingDescription.stride = createInfo.vertexBuffers[i].stride;
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            vertexBindingDescriptions.push_back(bindingDescription);
        }

        List<VkVertexInputAttributeDescription> vertexAttributeDescriptions{};
        for (u32 i = 0; i < createInfo.vertexBuffers.size(); i++)
        {
            for (auto& attribute : createInfo.vertexBuffers[i].attributes)
            {
                VkVertexInputAttributeDescription attributeDescription;
                attributeDescription.binding = i;
                attributeDescription.location = attribute.location;
                attributeDescription.format = VertexFormatToVk(attribute.format);
                attributeDescription.offset = attribute.offset;
                vertexAttributeDescriptions.push_back(attributeDescription);
            }
        }

        List<const Shader*> shaders = { vertexShader.get(), fragmentShader.get() };
        GraphicsPipelineInfo info{};
        info.depthTestEnable = createInfo.depthFormat.IsSome();
        info.cullMode = CullModeToVk(createInfo.cullMode);
        info.frontFace = FrontFaceToVk(createInfo.frontFace);
        info.primitiveTopology = PrimitiveTopologyToVk(createInfo.topology);
        info.vertexBindingDescriptions = vertexBindingDescriptions;
        info.vertexAttributeDescriptions = vertexAttributeDescriptions;

        List<PushConstantRange> pc{};
        
        auto& layoutIter = s->graphicsPipelineLayouts.find(graphicsPipelineHandle);
        BX_ENSURE(layoutIter != s->graphicsPipelineLayouts.end());

        graphicsPipeline = std::shared_ptr<GraphicsPipeline>(new GraphicsPipeline(
            s->device,
            shaders,
            s->activeRenderPass,
            layoutIter->second,
            pc, info));
        graphicsPipelineMap.insert(std::make_pair(s->activeRenderPassInfo.Unwrap(), graphicsPipeline));
    }
    else
    {
        graphicsPipeline = graphicsPipelineMapIter->second;
    }

    s->cmdList->BindGraphicsPipeline(graphicsPipeline);
}

void Graphics::SetVertexBuffer(u32 slot, const BufferSlice& bufferSlice)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ASSERT(slot == 0, "TODO");
    BX_ENSURE(bufferSlice.buffer);

    auto bufferIter = s->buffers.find(bufferSlice.buffer);
    BX_ENSURE(bufferIter != s->buffers.end());

    s->cmdList->BindVertexBuffer(bufferIter->second);
}

void Graphics::SetIndexBuffer(const BufferSlice& bufferSlice, IndexFormat format)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ENSURE(bufferSlice.buffer);

    auto bufferIter = s->buffers.find(bufferSlice.buffer);
    BX_ENSURE(bufferIter != s->buffers.end());

    s->boundIndexFormat = Optional<IndexFormat>::Some(format);

    s->cmdList->BindIndexBuffer(bufferIter->second, format == IndexFormat::UINT16 ? VkIndexType::VK_INDEX_TYPE_UINT16 : VkIndexType::VK_INDEX_TYPE_UINT32);
}

void Graphics::SetBindGroup(u32 index, BindGroupHandle bindGroup)
{
    BX_ASSERT(s->activeRenderPassHandle || s->activeComputePass, "No render pass or compute pass active.");
    BX_ENSURE(bindGroup);

    if (s->isRenderPassBound)
    {
        s->cmdList->EndRenderPass();
        s->isRenderPassBound = false;
    }

    auto bindGroupIter = s->bindGroups.find(bindGroup);
    BX_ENSURE(bindGroupIter != s->bindGroups.end());

    b8 isGraphics = s->activeRenderPassHandle;
    bindGroupIter->second->TransitionResourceStates(*s->cmdList, isGraphics);

    VkPipelineBindPoint bindPoint = s->activeRenderPassHandle ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
    s->cmdList->BindDescriptorSet(bindGroupIter->second, index, bindPoint);
}

void Graphics::Draw(u32 vertexCount, u32 firstVertex, u32 instanceCount)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ASSERT(instanceCount > 0, "Instance count must be larger than 0.");

    if (!s->isRenderPassBound)
    {
        s->cmdList->BeginRenderPass(s->activeRenderPass, s->renderPassFramebuffer, Color::Magenta());
        s->isRenderPassBound = true;
    }

    s->cmdList->Draw(vertexCount, instanceCount, firstVertex);
}

void Graphics::DrawIndexed(u32 indexCount, u32 instanceCount)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ASSERT(s->boundIndexFormat.IsSome(), "No index buffer bound.");
    BX_ASSERT(instanceCount > 0, "Instance count must be larger than 0.");

    if (!s->isRenderPassBound)
    {
        s->cmdList->BeginRenderPass(s->activeRenderPass, s->renderPassFramebuffer, Color::Magenta());
        s->isRenderPassBound = true;
    }

    s->cmdList->DrawElements(indexCount, instanceCount);
}

void Graphics::EndRenderPass(RenderPassHandle& renderPass)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ENSURE(renderPass);

    const RenderPassDescriptor& descriptor = Graphics::GetRenderPassDescriptor(renderPass);
    if (s->isRenderPassBound)
    {
        s->cmdList->EndRenderPass();
        s->isRenderPassBound = false;
    }

    s->activeRenderPassInfo.Reset();
    s->renderPassFramebuffer.reset();
    s->activeRenderPassHandle = RenderPassHandle::null;
    s->renderPassHandlePool.Destroy(renderPass);
    s_createInfoCache->renderPassCreateInfos.erase(renderPass);

    s->boundGraphicsPipeline = GraphicsPipelineHandle::null;
}

ComputePassHandle Graphics::BeginComputePass(const ComputePassDescriptor& descriptor)
{
    BX_ASSERT(!s->activeComputePass, "Compute pass already active.");
    BX_ASSERT(!s->activeRenderPassHandle, "Render pass already active.");

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

    auto pipelineIter = s->computePipelines.find(computePipeline);
    BX_ENSURE(pipelineIter != s->computePipelines.end());

    s->cmdList->BindComputePipeline(pipelineIter->second);
}

void Graphics::DispatchWorkgroups(u32 x, u32 y, u32 z)
{
    BX_ASSERT(s->activeComputePass, "No compute pass active.");
    BX_ASSERT(s->boundComputePipeline, "No compute pipeline bound.");

    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(s->cmdList->GetCommandBuffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    s->cmdList->Dispatch(x, y, z);
}

void Graphics::DispatchWorkgroupsIndirect(BufferHandle indirectArgs, u32 offset)
{
    BX_ASSERT(s->activeComputePass, "No compute pass active.");
    BX_ASSERT(s->boundComputePipeline, "No compute pipeline bound.");

    auto bufferIter = s->buffers.find(indirectArgs);
    BX_ENSURE(bufferIter != s->buffers.end());

    auto& createInfo = GetBufferCreateInfo(indirectArgs);
    BX_ASSERT(createInfo.usageFlags & BufferUsageFlags::INDIRECT, "Buffer must be created with BufferUsageFlags::INDIRECT when using as indirect args.");

    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(s->cmdList->GetCommandBuffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    //VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    vkCmdPipelineBarrier(s->cmdList->GetCommandBuffer(), VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    s->cmdList->DispatchIndirect(bufferIter->second, offset);
}

void Graphics::EndComputePass(ComputePassHandle& computePass)
{
    BX_ASSERT(s->activeComputePass, "No compute pass active.");
    BX_ENSURE(computePass);

    s->activeComputePass = ComputePassHandle::null;
    s->computePassHandlePool.Destroy(computePass);

    s->boundComputePipeline = ComputePipelineHandle::null;
}

void WriteBuffer(const std::shared_ptr<Buffer>& buffer, const void* data, const BufferCreateInfo& createInfo, Optional<SizeType> optionalSize, u32 offset)
{
    BX_ASSERT(createInfo.usageFlags & BufferUsageFlags::COPY_DST, "Buffer must be created with BufferUsageFlags::COPY_DST when writing to.");

    const u32 size = optionalSize.IsSome() ? optionalSize.Unwrap() : createInfo.size;

    std::shared_ptr<Buffer> stagingBuffer(new Buffer(Log::Format("Write {} Staging Buffer", createInfo.name), s->device,
        *s->physicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        static_cast<uint64_t>(size), BufferLocation::CPU_TO_GPU));

    void* bufferData = stagingBuffer->Map();
    memcpy(bufferData, data, size);
    stagingBuffer->Unmap();

    if (!s->uploadCmdList) s->uploadCmdList = s->cmdQueue->GetCmdList("Upload Cmd List");
    s->uploadCmdList->CopyBuffers(stagingBuffer, buffer, offset);
}

void Graphics::WriteBuffer(BufferHandle buffer, u64 offset, const void* data)
{
    BX_ENSURE(buffer && data);

    auto bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());
    auto& createInfo = GetBufferCreateInfo(buffer);

    ::WriteBuffer(bufferIter->second, data, createInfo, Optional<SizeType>::None(), offset);
}

void Graphics::WriteBuffer(BufferHandle buffer, u64 offset, const void* data, SizeType size)
{
    BX_ENSURE(buffer && data);

    auto bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());
    auto& createInfo = GetBufferCreateInfo(buffer);

    ::WriteBuffer(bufferIter->second, data, createInfo, Optional<SizeType>::Some(size), offset);
}

void Graphics::ClearBuffer(BufferHandle buffer)
{
    BX_ENSURE(buffer);

    auto bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());
    auto& createInfo = GetBufferCreateInfo(buffer);

    BX_ASSERT(createInfo.usageFlags & BufferUsageFlags::COPY_DST, "Buffer must be created with BufferUsageFlags::COPY_DST when clearing.");

    s->cmdList->FillBuffer(bufferIter->second, 0);
}

void Graphics::WriteTexture(TextureHandle texture, const void* data, const Extend3D& offset, const Extend3D& size)
{
    BX_ENSURE(texture && data);

    auto textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());
    auto createInfo = GetTextureCreateInfo(texture);

    BX_ASSERT(createInfo.usageFlags & TextureUsageFlags::COPY_DST, "Texture must be created with TextureUsageFlags::COPY_DST when writing to.");

    u32 sizeInBytes = SizeOfTexturePixels(createInfo);

    std::shared_ptr<Buffer> stagingBuffer(new Buffer(Log::Format("Write {} Staging Buffer", createInfo.name), s->device,
        *s->physicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        static_cast<uint64_t>(sizeInBytes), BufferLocation::CPU_TO_GPU));

    void* bufferData = stagingBuffer->Map();
    memcpy(bufferData, data, sizeInBytes);
    stagingBuffer->Unmap();

    if (!s->uploadCmdList) s->uploadCmdList = s->cmdQueue->GetCmdList("Upload Cmd List");
    s->uploadCmdList->TransitionImageLayout(textureIter->second, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
    s->uploadCmdList->CopyBuffers(stagingBuffer, textureIter->second);
}

void Graphics::ReadTexture(TextureHandle texture, void* data, const Extend3D& offset, const Extend3D& size)
{
    BX_ENSURE(texture);

    auto textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());
    auto createInfo = GetTextureCreateInfo(texture);

    BX_ASSERT(createInfo.usageFlags & TextureUsageFlags::COPY_SRC, "Texture must be created with TextureUsageFlags::COPY_SRC when reading from.");

    u32 pixelSizeInBytes = SizeOfTextureFormat(createInfo.format);
    u32 sizeInBytes = size.width * size.height * size.depthOrArrayLayers * pixelSizeInBytes;

    std::shared_ptr<Buffer> readbackBuffer(new Buffer(Log::Format("{} Readback Buffer", createInfo.name), s->device,
        *s->physicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        static_cast<uint64_t>(sizeInBytes), BufferLocation::CPU_TO_GPU));

    u32 offsetHeight = createInfo.size.height - offset.height;

    auto cmdList = s->cmdQueue->GetCmdList("Read Texture Cmd List");
    cmdList->TransitionImageLayout(textureIter->second, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
    cmdList->CopyBuffers(textureIter->second, readbackBuffer, VkOffset3D{ (i32)offset.width, (i32)offsetHeight, (i32)offset.depthOrArrayLayers }, VkExtent3D{size.width, size.height, size.depthOrArrayLayers});
    cmdList->TransitionImageLayout(textureIter->second, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    s->cmdQueue->SubmitCmdList(cmdList, nullptr, {}, {}, {});
    s->device->WaitIdle();

    void* bufferData = readbackBuffer->Map();
    memcpy(data, bufferData, sizeInBytes);
    readbackBuffer->Unmap();
}

// TODO: obliterate this obomination
void Graphics::DebugDraw(const Mat4& viewProj, const DebugDrawAttribs& attribs, const List<DebugVertex>& vertices)
{
}

// TODO: this can go when there's a imgui render impl using the higher level graphics module api

#include <backends/imgui_impl_vulkan.h>
ImGui_ImplVulkan_InitInfo GraphicsVulkan::ImGuiInitInfo()
{
    ImGui_ImplVulkan_InitInfo info{};
    info.Instance = s->instance->GetInstance();
    info.PhysicalDevice = s->physicalDevice->GetPhysicalDevice();
    info.Device = s->device->GetDevice();
    info.QueueFamily = s->physicalDevice->GraphicsFamily();
    info.Queue = s->cmdQueue->GetQueue();
    info.DescriptorPool = s->descriptorPool->GetPool();
    info.RenderPass = s->swapchain->GetRenderPass()->GetRenderPass(); // TODO: this breaks on resize
    info.MinImageCount = 2; // TODO: should this be properly queried from the swapchain?
    info.ImageCount = 2;
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.MinAllocationSize = 1024 * 1024;
    return info;
}

std::shared_ptr<DescriptorSet> GraphicsVulkan::TextureAsDescriptorSet(TextureHandle texture)
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::shared_ptr<DescriptorSetLayout> layout(new DescriptorSetLayout("Texture As Descriptor Set Layout", s->device, { binding }));
    std::shared_ptr<DescriptorSet> descriptorSet(new DescriptorSet("Texture As Descriptor Set", s->device, s->descriptorPool, layout));
    
    auto textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());
    descriptorSet->SetImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureIter->second, s->sampler);

    return descriptorSet;
}

u32 GraphicsVulkan::GetCurrentFrameIdx()
{
    return s->swapchain->GetCurrentFrameIdx();
}

VkCommandBuffer GraphicsVulkan::RawCommandBuffer()
{
    return s->cmdList->GetCommandBuffer();
}

void GraphicsVulkan::WaitIdle()
{
    s->device->WaitIdle();
}