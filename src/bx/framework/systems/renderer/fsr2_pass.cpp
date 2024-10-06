#include "bx/framework/systems/renderer/fsr2_pass.hpp"

#include "bx/framework/systems/renderer/lazy_init.hpp"

#include "bx/engine/core/file.hpp"
#include "bx/engine/core/shared_library.hpp"
#include "bx/engine/modules/graphics/backend/graphics_vulkan.hpp"

#include "bx/framework/components/transform.hpp"
#include "bx/framework/components/mesh_filter.hpp"
#include "bx/framework/components/mesh_renderer.hpp"

#include <ffx/shaders/ffx_fsr2_resources.h>
#include <ffx/vk/shaders/ffx_fsr2_shaders_vk.h>
#include <ffx/vk/ffx_fsr2_vk.h>

using fp_Fsr2ContextCreate = FfxErrorCode(*)(FfxFsr2Context* context, const FfxFsr2ContextDescription* contextDescription);
using fp_Fsr2ContextDestroy = FfxErrorCode(*)(FfxFsr2Context* context);
using fp_Fsr2ContextDispatch = FfxErrorCode(*)(FfxFsr2Context* context, const FfxFsr2DispatchDescription* dispatchDescription);

using fp_Fsr2GetScratchMemorySizeVK = size_t(*)(VkPhysicalDevice physicalDevice);
using fp_Fsr2GetInterfaceVK = FfxErrorCode(*)(FfxFsr2Interface* outInterface, void* scratchBuffer, size_t scratchBufferSize, VkPhysicalDevice physicalDevice, PFN_vkGetDeviceProcAddr getDeviceProcAddr);

struct Fsr2State
{
    fp_Fsr2ContextCreate fsr2ContextCreate;
    fp_Fsr2ContextDestroy fsr2ContextDestroy;
    fp_Fsr2ContextDispatch fsr2ContextDispatch;

    fp_Fsr2GetScratchMemorySizeVK fsr2GetScratchMemorySizeVK;
    fp_Fsr2GetInterfaceVK fsr2GetInterfaceVK;
};

struct Fsr2 : public LazyInit<Fsr2, Fsr2State>
{
    Fsr2()
    {
        SharedLibrary fsrLibrary("ffx_fsr2_api_x64");
        SharedLibrary fsrVkLibrary("ffx_fsr2_api_vk_x64");
        
        data.fsr2ContextCreate = fsrLibrary.LoadFunction<fp_Fsr2ContextCreate>("ffxFsr2ContextCreate");
        BX_ENSURE(data.fsr2ContextCreate);
        data.fsr2ContextDestroy = fsrLibrary.LoadFunction<fp_Fsr2ContextDestroy>("ffxFsr2ContextDestroy");
        BX_ENSURE(data.fsr2ContextDestroy);
        data.fsr2ContextDispatch = fsrLibrary.LoadFunction<fp_Fsr2ContextDispatch>("ffxFsr2ContextDispatch");
        BX_ENSURE(data.fsr2ContextDispatch);

        data.fsr2GetScratchMemorySizeVK = fsrVkLibrary.LoadFunction<fp_Fsr2GetScratchMemorySizeVK>("ffxFsr2GetScratchMemorySizeVK");
        BX_ENSURE(data.fsr2GetScratchMemorySizeVK);
        data.fsr2GetInterfaceVK = fsrVkLibrary.LoadFunction<fp_Fsr2GetInterfaceVK>("ffxFsr2GetInterfaceVK");
        BX_ENSURE(data.fsr2GetInterfaceVK);
    }
};

template<>
std::unique_ptr<Fsr2> LazyInit<Fsr2, Fsr2State>::cache = nullptr;

void CheckFsr(FfxErrorCode err)
{
    if (err)
    {
        if (err == FFX_ERROR_INVALID_POINTER)
            BX_FAIL("Fsr2 operation failed due to an invalid pointer");
        else if (err == FFX_ERROR_INVALID_ALIGNMENT)
            BX_FAIL("Fsr2 operation failed due to an invalid alignment.");
        else if (err == FFX_ERROR_INVALID_SIZE)
            BX_FAIL("Fsr2 operation failed due to an invalid size.");
        else if (err == FFX_EOF)
            BX_FAIL("Fsr2 end of the file was encountered.");
        else if (err == FFX_ERROR_INVALID_PATH)
            BX_FAIL("Fsr2 operation failed because the specified path was invalid.");
        else if (err == FFX_ERROR_EOF)
            BX_FAIL("Fsr2 operation failed because end of file was reached.");
        else if (err == FFX_ERROR_MALFORMED_DATA)
            BX_FAIL("Fsr2 operation failed because of some malformed data.");
        else if (err == FFX_ERROR_OUT_OF_MEMORY)
            BX_FAIL("Fsr2 operation failed because it ran out memory.");
        else if (err == FFX_ERROR_INCOMPLETE_INTERFACE)
            BX_FAIL("Fsr2 operation failed because the interface was not fully configured.");
        else if (err == FFX_ERROR_INVALID_ENUM)
            BX_FAIL("Fsr2 operation failed because of an invalid enumeration value.");
        else if (err == FFX_ERROR_INVALID_ARGUMENT)
            BX_FAIL("Fsr2 operation failed because an argument was invalid.");
        else if (err == FFX_ERROR_OUT_OF_RANGE)
            BX_FAIL("Fsr2 operation failed because a value was out of range.");
        else if (err == FFX_ERROR_NULL_DEVICE)
            BX_FAIL("Fsr2 operation failed because a device was null.");
        else if (err == FFX_ERROR_BACKEND_API_ERROR)
            BX_FAIL("Fsr2 operation failed because the backend API returned an error code.");
        else if (err == FFX_ERROR_INSUFFICIENT_MEMORY)
            BX_FAIL("Fsr2 operation failed because there was not enough memory.");
    }
}

FfxSurfaceFormat FfxSurfaceFormatToVk(VkFormat fmt)
{
    switch (fmt) {

    case(VK_FORMAT_R32G32B32A32_SFLOAT):
        return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case(VK_FORMAT_R16G16B16A16_SFLOAT):
        return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case(VK_FORMAT_R16G16B16A16_UNORM):
        return FFX_SURFACE_FORMAT_R16G16B16A16_UNORM;
    case(VK_FORMAT_R32G32_SFLOAT):
        return FFX_SURFACE_FORMAT_R32G32_FLOAT;
    case(VK_FORMAT_R32_UINT):
        return FFX_SURFACE_FORMAT_R32_UINT;
    case(VK_FORMAT_R8G8B8A8_UNORM):
        return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case(VK_FORMAT_B10G11R11_UFLOAT_PACK32):
        return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
    case(VK_FORMAT_R16G16_SFLOAT):
        return FFX_SURFACE_FORMAT_R16G16_FLOAT;
    case(VK_FORMAT_R16G16_UINT):
        return FFX_SURFACE_FORMAT_R16G16_UINT;
    case(VK_FORMAT_R16_SFLOAT):
        return FFX_SURFACE_FORMAT_R16_FLOAT;
    case(VK_FORMAT_R16_UINT):
        return FFX_SURFACE_FORMAT_R16_UINT;
    case(VK_FORMAT_R16_UNORM):
        return FFX_SURFACE_FORMAT_R16_UNORM;
    case(VK_FORMAT_R16_SNORM):
        return FFX_SURFACE_FORMAT_R16_SNORM;
    case(VK_FORMAT_R8_UNORM):
        return FFX_SURFACE_FORMAT_R8_UNORM;
    case(VK_FORMAT_R32_SFLOAT):
        return FFX_SURFACE_FORMAT_R32_FLOAT;
    case(VK_FORMAT_R8_UINT):
        return FFX_SURFACE_FORMAT_R8_UINT;
    default:
        return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

FfxResource TextureToFfx(TextureHandle texture, TextureViewHandle textureView, FfxResourceStates state)
{
    FfxResource resource{};

    auto& createInfo = Graphics::GetTextureCreateInfo(texture);
    auto image = GraphicsVulkan::GetImage(texture);
    auto imageView = GraphicsVulkan::GetImageView(textureView);

    resource.resource = reinterpret_cast<void*>(image->GetImage());
    resource.state = state;
    resource.isDepth = IsTextureFormatDepth(createInfo.format);
    resource.descriptorData = reinterpret_cast<u64>(imageView->GetImageView());
    resource.description.flags = FFX_RESOURCE_FLAGS_NONE;
    resource.description.type = FFX_RESOURCE_TYPE_TEXTURE2D;
    resource.description.width = createInfo.size.width;
    resource.description.height = createInfo.size.height;
    resource.description.depth = 1;
    resource.description.mipCount = 1;
    resource.description.format = FfxSurfaceFormatToVk(image->Format());

    return resource;
}

Fsr2Pass::Fsr2Pass(u32 width, u32 height, u32 outputWidth, u32 outputHeight)
    : width(width), height(height), outputWidth(outputWidth), outputHeight(outputHeight)
{
    const Fsr2State& fsr2 = Fsr2::Get();

    FfxFsr2ContextDescription contextDescription{};
    const size_t scratchBufferSize = fsr2.fsr2GetScratchMemorySizeVK(GraphicsVulkan::GetPhysicalDevice().GetPhysicalDevice());
    scratchBuffer = malloc(scratchBufferSize);
    CheckFsr(fsr2.fsr2GetInterfaceVK(&contextDescription.callbacks, scratchBuffer, scratchBufferSize, GraphicsVulkan::GetPhysicalDevice().GetPhysicalDevice(), vkGetDeviceProcAddr));

    contextDescription.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
#ifdef BX_DEBUG_BUILD
    contextDescription.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
#endif
    contextDescription.maxRenderSize.width = width;
    contextDescription.maxRenderSize.height = height;
    contextDescription.displaySize.width = outputWidth;
    contextDescription.displaySize.height = outputHeight;
    contextDescription.device = reinterpret_cast<FfxDevice>(GraphicsVulkan::GetDevice()->GetDevice());

    CheckFsr(fsr2.fsr2ContextCreate(&fsr2Context, &contextDescription));

    TextureCreateInfo outputTargetCreateInfo{};
    outputTargetCreateInfo.name = "Fsr2 Output Target";
    outputTargetCreateInfo.size = Extend3D(outputWidth, outputHeight, 1);
    outputTargetCreateInfo.format = TextureFormat::RGBA32_FLOAT;
    outputTargetCreateInfo.usageFlags = TextureUsageFlags::STORAGE_BINDING | TextureUsageFlags::TEXTURE_BINDING;
    outputTarget = Graphics::CreateTexture(outputTargetCreateInfo);
}

Fsr2Pass::~Fsr2Pass()
{
    const Fsr2State& fsr2 = Fsr2::Get();

    free(scratchBuffer);

    CheckFsr(fsr2.fsr2ContextDestroy(&fsr2Context));

    Graphics::DestroyTexture(outputTarget);
}

TextureHandle Fsr2Pass::GetResolvedColorTarget() const
{
    return outputTarget;
}

void Fsr2Pass::Dispatch(const Camera& camera, TextureHandle colorTarget, TextureHandle depthTarget, TextureHandle velocityTarget)
{
    auto colorTargetView = Graphics::CreateTextureView(colorTarget);
    auto depthTargetView = Graphics::CreateTextureView(depthTarget);
    auto velocityTargetView = Graphics::CreateTextureView(velocityTarget);
    auto outputTargetView = Graphics::CreateTextureView(outputTarget);

    auto cmdList = GraphicsVulkan::GetCurrentCommandList();
    cmdList->TransitionImageLayout(GraphicsVulkan::GetImage(colorTarget), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    cmdList->TransitionImageLayout(GraphicsVulkan::GetImage(depthTarget), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    cmdList->TransitionImageLayout(GraphicsVulkan::GetImage(velocityTarget), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    cmdList->TransitionImageLayout(GraphicsVulkan::GetImage(outputTarget), VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    const Fsr2State& fsr2 = Fsr2::Get();

    FfxFsr2DispatchDescription dispatchDescription{};
    dispatchDescription.commandList = reinterpret_cast<FfxCommandList>(GraphicsVulkan::GetCurrentCommandList()->GetCommandBuffer());
    dispatchDescription.color = TextureToFfx(colorTarget, colorTargetView, FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchDescription.depth = TextureToFfx(depthTarget, depthTargetView, FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchDescription.motionVectors = TextureToFfx(velocityTarget, velocityTargetView, FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchDescription.output = TextureToFfx(outputTarget, outputTargetView, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    dispatchDescription.jitterOffset.x = -camera.GetJitter().x;
    dispatchDescription.jitterOffset.y = -camera.GetJitter().y;
    dispatchDescription.motionVectorScale.x = width;
    dispatchDescription.motionVectorScale.y = height;
    dispatchDescription.renderSize.width = width;
    dispatchDescription.renderSize.height = height;
    dispatchDescription.enableSharpening = true;
    dispatchDescription.sharpness = 0.3;
    dispatchDescription.frameTimeDelta = (1.0 / 60.0) * 1000.0; // TODO
    dispatchDescription.preExposure = 1.0;
    dispatchDescription.reset = false;
    dispatchDescription.cameraNear = camera.GetZNear();
    dispatchDescription.cameraFar = camera.GetZFar();
    f32 verticalFov = 2.0 * atanf(tanf(camera.GetFov() / 2.0) / camera.GetAspect());
    dispatchDescription.cameraFovAngleVertical = verticalFov;
    dispatchDescription.viewSpaceToMetersFactor = 1.0;
    dispatchDescription.enableAutoReactive = false;

    CheckFsr(fsr2.fsr2ContextDispatch(&fsr2Context, &dispatchDescription));
}

void Fsr2Pass::ClearPipelineCache()
{
    
}