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

#include "bx/engine/modules/graphics/backend/vulkan/conversion.hpp"
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

static String PRESENT_VERT_SRC = R"""(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 fragTexCoord;

void main() {
    fragTexCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(fragTexCoord * 2.0f + -1.0f, 0.0f, 1.0f);
    fragTexCoord.y = -fragTexCoord.y;
}
)""";

static String PRESENT_FRAG_SRC = R"""(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D colorImage;

void main() {
    vec3 color = texture(colorImage, fragTexCoord).rgb;
    outColor = vec4(color, 1.0);
}
)""";

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

    HashMap<BufferHandle, std::shared_ptr<Buffer>> buffers;
    HashMap<TextureHandle, std::shared_ptr<Image>> textures;
    HashMap<TextureViewHandle, TextureView> textureViews;
    HashMap<ShaderHandle, std::shared_ptr<Shader>> shaders;
    HashMap<GraphicsPipelineHandle, HashMap<RenderPassInfo, std::shared_ptr<GraphicsPipeline>>> graphicsPipelines;
    HashMap<GraphicsPipelineHandle, const List<std::shared_ptr<DescriptorSetLayout>>> graphicsPipelineLayouts;
    HashMap<BindGroupHandle, std::shared_ptr<DescriptorSet>> bindGroups;

    std::shared_ptr<Framebuffer> renderPassFramebuffer = nullptr;
    std::shared_ptr<RenderPass> activeRenderPass = nullptr;
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

    std::shared_ptr<Image> colorImage;
    std::shared_ptr<Image> depthImage;
    std::unique_ptr<Framebuffer> framebuffer;
    std::shared_ptr<RenderPass> renderPass;
    std::shared_ptr<Sampler> sampler;

    std::shared_ptr<GraphicsPipeline> presentPipeline;
    std::shared_ptr<DescriptorSetLayout> presentDescriptorSetLayout;
    Array<std::shared_ptr<DescriptorSet>, Swapchain::MAX_FRAMES_IN_FLIGHT> presentDescriptorSets = { nullptr, nullptr };

    std::shared_ptr<Fence> presentFence;
    std::shared_ptr<CmdList> cmdList;
};
static std::unique_ptr<State> s;

struct RenderPassCache : public LazyInitMap<RenderPassCache, std::shared_ptr<RenderPass>, RenderPassInfo>
{
    RenderPassCache(const RenderPassInfo& args)
    {
        data = std::shared_ptr<RenderPass>(new RenderPass("render pass", s->device, args));
    }
};

template<>
HashMap<RenderPassInfo, std::unique_ptr<RenderPassCache>> LazyInitMap<RenderPassCache, std::shared_ptr<RenderPass>, RenderPassInfo>::cache = {};

void BuildSwapchain()
{
    i32 width, height;
    Window::GetSize(&width, &height);

    s->swapchain.reset();
    s->swapchain = std::make_unique<Swapchain>(static_cast<u32>(width), static_cast<u32>(height), *s->instance, s->device, *s->physicalDevice);
}

void BuildRenderTargets()
{
    i32 width, height;
    Window::GetSize(&width, &height);

    s->colorImage = std::make_shared<Image>(
        "Color Image", s->device, *s->physicalDevice, width, height, 1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_FORMAT_R16G16B16A16_SFLOAT);
    s->depthImage = std::make_shared<Image>(
        "Depth Image", s->device, *s->physicalDevice, width, height, 1,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_FORMAT_D24_UNORM_S8_UINT);

    RenderPassInfo renderPassInfo{};
    renderPassInfo.colorFormats = { s->colorImage->Format() };
    renderPassInfo.depthFormat = Optional<VkFormat>::Some(s->depthImage->Format());
    s->renderPass = std::make_shared<RenderPass>("Main Render Pass",
        s->device, renderPassInfo);

    FramebufferInfo framebufferInfo{};
    framebufferInfo.images = { s->colorImage, s->depthImage };
    framebufferInfo.renderPass = s->renderPass;
    s->framebuffer = std::make_unique<Framebuffer>("Main Framebuffer", s->device, framebufferInfo);
}

void BuildDescriptors()
{
    VkDescriptorSetLayoutBinding presentBinding0{};
    presentBinding0.binding = 0;
    presentBinding0.descriptorCount = 1;
    presentBinding0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    presentBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    s->presentDescriptorSetLayout = std::make_shared<DescriptorSetLayout>(
        "Present Descriptor Set Layout 0",
        s->device, List<VkDescriptorSetLayoutBinding>{ presentBinding0 });

    for (size_t i = 0; i < s->presentDescriptorSets.size(); i++) {
        s->presentDescriptorSets[i] = std::make_shared<DescriptorSet>(
            "Present Descriptor Set 0",
            s->device, s->descriptorPool, s->presentDescriptorSetLayout);
    }
}

void BuildPipelines()
{
    std::shared_ptr<Shader> presentVertexShader =
        std::make_shared<Shader>("present vert", s->device, VK_SHADER_STAGE_VERTEX_BIT, PRESENT_VERT_SRC);
    std::shared_ptr<Shader> presentFragmentShader =
        std::make_shared<Shader>("present frag", s->device, VK_SHADER_STAGE_FRAGMENT_BIT, PRESENT_FRAG_SRC);

    GraphicsPipelineInfo presentInfo{};
    presentInfo.ignoreDepth = true;
    presentInfo.inputVertices = false;
    std::vector<const Shader*> presentShaders = { presentVertexShader.get(),
                                                 presentFragmentShader.get() };
    s->presentPipeline = std::make_shared<GraphicsPipeline>(
        s->device, presentShaders, s->swapchain->GetRenderPass(),
        std::vector<std::shared_ptr<DescriptorSetLayout>>{s->presentDescriptorSetLayout},
        std::vector<PushConstantRange>{}, presentInfo);
}

bool Graphics::Initialize()
{
    s_createInfoCache = std::unique_ptr<CreateInfoCache>(new CreateInfoCache());
    s = std::make_unique<State>();

#ifdef BX_WINDOW_GLFW_BACKEND
    GLFWwindow* glfwWindow = WindowGLFW::GetWindowPtr();

    s->instance = std::make_shared<Instance>((void*)glfwWindow, ENABLE_VALIDATION);
#else

    BX_LOGE("Window backend not supported!");
    return false;
#endif
    
    s->physicalDevice = std::make_unique<PhysicalDevice>(*s->instance);
    s->device = std::make_shared<Device>(s->instance, *s->physicalDevice, ENABLE_VALIDATION);
    s->cmdQueue = std::make_unique<CmdQueue>(s->device, *s->physicalDevice, QueueType::GRAPHICS);
    s->descriptorPool = std::make_shared<DescriptorPool>(s->device);
    s->sampler = std::make_shared<Sampler>("Sampler", s->device, *s->physicalDevice,
        SamplerInfo{});

    BuildSwapchain();
    for (u32 i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        TextureHandle textureHandle = s->textureHandlePool.Create();
        s_createInfoCache->textureCreateInfos.insert(std::make_pair(textureHandle, s->swapchain->GetImageCreateInfo()));

        std::shared_ptr<Image> image = s->swapchain->GetImage(i);
        s->textures.emplace(textureHandle, image);
        s->swapchainColorTargets[i] = textureHandle;
        s->swapchainColorTargetViews[i] = TextureViewHandle{ textureHandle.id };
    }
    
    BuildRenderTargets();
    BuildDescriptors();
    BuildPipelines();

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
    s->cmdQueue->ProcessCmdLists();

    if (Window::IsActive())
    {
        if (Window::WasResized())
        {
            BuildSwapchain();
            for (u32 i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
            {
                TextureHandle textureHandle = s->textureHandlePool.Create();
                s_createInfoCache->textureCreateInfos.insert(std::make_pair(textureHandle, s->swapchain->GetImageCreateInfo()));

                std::shared_ptr<Image> image = s->swapchain->GetImage(i);
                s->textures.emplace(textureHandle, image);
            }

            BuildRenderTargets();
            BuildPipelines();
        }

        s->presentFence = s->swapchain->NextImage();

        // All cmds of the entire frame will be recorded into a single cmd list
        // This is because we designed the graphics module api to act like it's immediate
        s->cmdList = s->cmdQueue->GetCmdList();
    }
}

void Graphics::EndFrame()
{
    if (Window::IsActive())
    {
        // TODO: all rendering can happen before the image is available if we create a seperate present blit pipeline
        // This can also act as a hdr to sdr conversion and enable us to render in hdr

        Rect2D swapchainExtent = s->swapchain->Extent();
        size_t currentFrame = static_cast<size_t>(s->swapchain->GetCurrentFrameIdx());

        // Swapchain present pass
        s->cmdList->BeginRenderPass(s->renderPass, *s->framebuffer,
            Color(0.6f, 0.8f, 1.0f, 1.0f));
        Rect2D imgExtent(static_cast<float>(s->colorImage->Width()),
            static_cast<float>(s->colorImage->Height()));
        s->cmdList->SetScissor(imgExtent);
        s->cmdList->SetViewport(imgExtent);
        // TODO: render da shit
        s->cmdList->EndRenderPass();

        s->cmdList->TransitionImageLayout(s->colorImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        s->cmdList->BeginRenderPass(s->swapchain->GetRenderPass(),
            s->swapchain->GetCurrentFramebuffer(),
            Color(0.1f, 0.1f, 0.1f, 1.0f));
        s->cmdList->SetScissor(swapchainExtent);
        s->cmdList->SetViewport(swapchainExtent);
        s->cmdList->BindGraphicsPipeline(s->presentPipeline);
        s->presentDescriptorSets[currentFrame]->SetImage(
            0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, s->colorImage, s->sampler);
        s->cmdList->BindDescriptorSet(s->presentDescriptorSets[currentFrame], 0);
        s->cmdList->Draw(3);
        ImGuiImpl::EndFrame();
        s->cmdList->EndRenderPass();
        s->cmdList->TransitionImageLayout(
            s->colorImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        // Execute all rendering cmds when the image is available
        List<Semaphore*> waitSemaphores{ &s->swapchain->GetImageAvailableSemaphore() };
        List<VkPipelineStageFlags> presentWaitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        List<Semaphore*> presentSignalSemaphores{
            &s->swapchain->GetRenderFinishedSemaphore() };
        s->cmdQueue->SubmitCmdList(s->cmdList, s->presentFence, waitSemaphores, presentWaitStages,
            presentSignalSemaphores);

        // Present when rendering is finished, indicated by the `presentSignalSemaphores`
        s->swapchain->Present(*s->cmdQueue, *s->presentFence, presentSignalSemaphores);
    }

    s->cmdList.reset();
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

TextureHandle Graphics::CreateTexture(const TextureCreateInfo& createInfo)
{
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
        // TODO!
        // WriteTexture(textureHandle, 0, createInfo.data);
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

BufferHandle Graphics::CreateBuffer(const BufferCreateInfo& createInfo)
{
    BX_ENSURE(ValidateBufferCreateInfo(createInfo));

    BufferHandle bufferHandle = s->bufferHandlePool.Create();
    s_createInfoCache->bufferCreateInfos.insert(std::make_pair(bufferHandle, createInfo.WithoutData()));

    VkBufferUsageFlags usage = BufferUsageFlagsToVk(createInfo.usageFlags);
    BufferLocation location = IsBufferUsageMappable(createInfo.usageFlags) ? BufferLocation::CPU_TO_GPU : BufferLocation::GPU_ONLY;

    std::shared_ptr<Buffer> buffer(new Buffer(createInfo.name, s->device, *s->physicalDevice, usage, createInfo.size, location));
    s->buffers.emplace(bufferHandle, buffer);

    // TODO: host visible (mappable) memory doesn't really benefit from using staging buffers, they should directly write to the mappable memory
    if (createInfo.data)
    {
        // TODO!
        // WriteBuffer(bufferHandle, 0, createInfo.data);
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

    VkShaderStageFlagBits stage = ShaderTypeToVk(createInfo.shaderType);

    std::shared_ptr<Shader> shader(new Shader(createInfo.name, s->device, stage, meta + createInfo.src));
    s->shaders.emplace(shaderHandle, shader);

    return shaderHandle;
}

void Graphics::DestroyShader(ShaderHandle& shader)
{
    BX_ENSURE(shader);

    auto shaderIter = s->shaders.find(shader);
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
    s_createInfoCache->graphicsPipelineCreateInfos.erase(graphicsPipeline);
    s->graphicsPipelineHandlePool.Destroy(graphicsPipeline);
}

ComputePipelineHandle Graphics::CreateComputePipeline(const ComputePipelineCreateInfo& createInfo)
{
    BX_ENSURE(ValidateComputePipelineCreateInfo(createInfo));

    ComputePipelineHandle computePipelineHandle = s->computePipelineHandlePool.Create();
    s_createInfoCache->computePipelineCreateInfos.insert(std::make_pair(computePipelineHandle, createInfo));

    /*auto shaderIter = s->shaders.find(createInfo.shader);
    BX_ENSURE(shaderIter != s->shaders.end());*/

    BX_FAIL("TODO");

    return computePipelineHandle;
}

void Graphics::DestroyComputePipeline(ComputePipelineHandle& computePipeline)
{
    BX_ENSURE(computePipeline);

   /* s->computePipelines.erase(computePipeline);
    s_createInfoCache->computePipelineCreateInfos.erase(computePipeline);
    s->computePipelineHandlePool.Destroy(computePipeline);*/
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
        BX_FAIL("TODO");
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

            // TODO: read std::shared_ptr<DescriptorSetLayout> layout and cache some stuff to figure out what the buffer type is
            descriptorSet->SetBuffer(entry.binding, VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, bufferIter->second);
            break;
        }
        case BindingResourceType::TEXTURE_VIEW:
        {
            auto textureViewIter = s->textureViews.find(entry.resource.textureView);
            BX_ENSURE(textureViewIter != s->textureViews.end());

            // TODO: read std::shared_ptr<DescriptorSetLayout> layout and cache some stuff to figure out what the buffer type is
            descriptorSet->SetImage(entry.binding, VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureViewIter->second.texture, s->sampler);
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

    s_createInfoCache->bindGroupCreateInfos.erase(bindGroup);
    s->bindGroupHandlePool.Destroy(bindGroup);
}

RenderPassHandle Graphics::BeginRenderPass(const RenderPassDescriptor& descriptor)
{
    BX_ASSERT(!s->activeRenderPassHandle, "Render pass already active.");

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

        VkFormat format = TextureFormatToVk(textureCreateInfo.format);
        renderPassInfo.depthFormat = Optional<VkFormat>::Some(format);

        if (width == 0 && height == 0)
        {
            auto createInfo = GetTextureCreateInfo(textureViewIter->second.handle);
            width = createInfo.size.width;
            height = createInfo.size.height;
        }
    }
    framebufferInfo.renderPass = RenderPassCache::Get(renderPassInfo);
    
    std::shared_ptr<Framebuffer> framebuffer(new Framebuffer("framebuffer", s->device, framebufferInfo));
    s->cmdList->BeginRenderPass(framebufferInfo.renderPass, *framebuffer, Color::Magenta());
    s->cmdList->SetViewport(Rect2D(width, height));
    s->cmdList->SetScissor(Rect2D(width, height));

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
        GraphicsPipelineInfo info{}; // TODO!
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

    auto bindGroupIter = s->bindGroups.find(bindGroup);
    BX_ENSURE(bindGroupIter != s->bindGroups.end());

    // TODO: compute
    s->cmdList->BindDescriptorSet(bindGroupIter->second, index, VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void Graphics::Draw(u32 vertexCount, u32 firstVertex, u32 instanceCount)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ASSERT(instanceCount > 0, "Instance count must be larger than 0.");

    s->cmdList->Draw(vertexCount, instanceCount, firstVertex);
}

void Graphics::DrawIndexed(u32 indexCount, u32 instanceCount)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ASSERT(s->boundGraphicsPipeline, "No graphics pipeline bound.");
    BX_ASSERT(s->boundIndexFormat.IsSome(), "No index buffer bound.");
    BX_ASSERT(instanceCount > 0, "Instance count must be larger than 0.");

    s->cmdList->DrawElements(indexCount, instanceCount);
}

void Graphics::EndRenderPass(RenderPassHandle& renderPass)
{
    BX_ASSERT(s->activeRenderPassHandle, "No render pass active.");
    BX_ENSURE(renderPass);

    const RenderPassDescriptor& descriptor = Graphics::GetRenderPassDescriptor(renderPass);
    s->cmdList->EndRenderPass();

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

    /*auto pipelineIter = s->computePipelines.find(computePipeline);
    BX_ENSURE(pipelineIter != s->computePipelines.end());
    auto& pipeline = pipelineIter->second;*/

    BX_FAIL("TODO");
}

void Graphics::DispatchWorkgroups(u32 x, u32 y, u32 z)
{
    BX_ASSERT(s->activeComputePass, "No compute pass active.");
    BX_ASSERT(s->boundComputePipeline, "No compute pipeline bound.");

    BX_FAIL("TODO");
}

void Graphics::EndComputePass(ComputePassHandle& computePass)
{
    BX_ASSERT(s->activeComputePass, "No compute pass active.");
    BX_ENSURE(computePass);

    s->activeComputePass = ComputePassHandle::null;
    s->computePassHandlePool.Destroy(computePass);

    s->boundComputePipeline = ComputePipelineHandle::null;
}

void WriteBuffer(const std::shared_ptr<Buffer>& buffer, const void* data, const BufferCreateInfo& createInfo, Optional<SizeType> size)
{
    BX_ASSERT(createInfo.usageFlags & BufferUsageFlags::COPY_DST, "Buffer must be created with BufferUsageFlags::COPY_DST when writing to.");

    std::shared_ptr<Buffer> stagingBuffer(new Buffer(Log::Format("Write {} Staging Buffer", createInfo.name), s->device,
        *s->physicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        static_cast<uint64_t>(createInfo.size), BufferLocation::CPU_TO_GPU));

    void* bufferData = stagingBuffer->Map();
    memcpy(bufferData, data, size.IsSome() ? size.Unwrap() : createInfo.size);
    stagingBuffer->Unmap();

    s->cmdList->CopyBuffers(stagingBuffer, buffer);
}

void Graphics::WriteBuffer(BufferHandle buffer, u64 offset, const void* data)
{
    BX_ENSURE(buffer && data);
    BX_ASSERT(offset == 0, "Offset must be 0 for now.");

    auto bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());
    auto& createInfo = GetBufferCreateInfo(buffer);

    ::WriteBuffer(bufferIter->second, data, createInfo, Optional<SizeType>::None());
}

void Graphics::WriteBuffer(BufferHandle buffer, u64 offset, const void* data, SizeType size)
{
    BX_ENSURE(buffer && data);
    BX_ASSERT(offset == 0, "Offset must be 0 for now.");

    auto bufferIter = s->buffers.find(buffer);
    BX_ENSURE(bufferIter != s->buffers.end());
    auto& createInfo = GetBufferCreateInfo(buffer);

    ::WriteBuffer(bufferIter->second, data, createInfo, Optional<SizeType>::Some(size));
}

void Graphics::WriteTexture(TextureHandle texture, const void* data, const Extend3D& offset, const Extend3D& size)
{
    BX_ENSURE(texture && data);

    /*auto textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());*/

    auto createInfo = GetTextureCreateInfo(texture);

    BX_FAIL("TODO");
}

void Graphics::ReadTexture(TextureHandle texture, void* data, const Extend3D& offset, const Extend3D& size)
{
    BX_ENSURE(texture);

    /*auto textureIter = s->textures.find(texture);
    BX_ENSURE(textureIter != s->textures.end());

    auto createInfo = GetTextureCreateInfo(texture);*/

    BX_FAIL("TODO");
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

VkCommandBuffer GraphicsVulkan::RawCommandBuffer()
{
    return s->cmdList->GetCommandBuffer();
}

void GraphicsVulkan::WaitIdle()
{
    s->device->WaitIdle();
}