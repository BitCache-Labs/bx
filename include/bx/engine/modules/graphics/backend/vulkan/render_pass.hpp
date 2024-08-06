#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/hash.hpp"
#include "bx/engine/containers/optional.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/string.hpp"

#include "vulkan_api.hpp"

#include <memory>

namespace Vk
{
    class Device;

    struct RenderPassInfo
    {
        List<VkFormat> colorFormats;
        Optional<VkFormat> depthFormat = Optional<VkFormat>::None();

        b8 operator==(const RenderPassInfo& other) const
        {
            if (colorFormats.size() != other.colorFormats.size())
                return false;

            for (u32 i = 0; i < colorFormats.size(); i++)
            {
                if (colorFormats[i] != other.colorFormats[i])
                    return false;
            }

            return depthFormat == other.depthFormat;
        }
    };

    class RenderPass : NoCopy {
    public:
        RenderPass(const String& name,
            std::shared_ptr<Device> device,
            const RenderPassInfo& info);
        ~RenderPass();

        VkRenderPass GetRenderPass() const;

    private:
        VkRenderPass renderPass;

        const std::shared_ptr<Device> device;
    };
}

template <>
struct std::hash<Vk::RenderPassInfo>
{
	std::size_t operator()(const Vk::RenderPassInfo& v) const
	{
        SizeType result = 0;
        for (auto& format : v.colorFormats)
            hashCombine(result, format);
        if (v.depthFormat.IsSome())
            hashCombine(result, v.depthFormat.Unwrap());
		return result;
	}
};

template <>
struct std::hash<Vk::RenderPass>
{
    std::size_t operator()(const Vk::RenderPass& v) const
    {
        SizeType result = 0;
        hashCombine(result, v.GetRenderPass());
        return result;
    }
};