#ifndef BXL_HPP
#define BXL_HPP

#include <bx.hpp>

#ifdef BX_VULKAN_SUPPORTED
#include <vulkan/vulkan.h>
#endif

namespace bx
{
	struct handle_t
	{
		handle_t() = default;

		explicit handle_t(const handle_id value)
			: id(value) {}

		handle_t(const u32 data, const u32 meta)
			: data(data)
			, meta(meta) {}

		explicit operator handle_id() const { return id; }

		template<typename T>
		T get_data() const { return static_cast<T>(data); }

		template<typename T>
		T get_meta() const { return static_cast<T>(meta); }

		union
		{
			handle_id id{};

			struct
			{
				u32 data;
				u32 meta;
			};
		};
	};

#ifdef BX_VULKAN_SUPPORTED
	extern VkAllocationCallbacks* g_allocator;
	extern VkInstance g_instance;
	extern VkSurfaceKHR g_surface;

	extern VkPhysicalDevice g_physical_device;
	extern VkDevice g_device;
	extern u32 g_queue_family;
	extern VkQueue g_queue;
	extern VkPipelineCache g_pipeline_cache;
	extern VkDescriptorPool g_descriptor_pool;

	extern VkFormat g_swapchain_format;

	extern VkCommandBuffer g_imgui_cmd_buffer;
	extern VkPipeline g_imgui_pipeline;
	extern VkRenderPass g_imgui_render_pass;
#endif
}

// Enum helpers
constexpr bx::gfx_shader_stage operator|(bx::gfx_shader_stage a, bx::gfx_shader_stage b) noexcept
{
	return static_cast<bx::gfx_shader_stage>(static_cast<u32>(a) | static_cast<u32>(b));
}

constexpr bx::gfx_shader_stage operator&(bx::gfx_shader_stage a, bx::gfx_shader_stage b) noexcept
{
	return static_cast<bx::gfx_shader_stage>(static_cast<u32>(a) & static_cast<u32>(b));
}

constexpr bool bx_enum_any(bx::gfx_shader_stage a) noexcept
{
	return static_cast<u32>(a) != 0;
}

constexpr bx::gfx_buffer_usage operator|(bx::gfx_buffer_usage a, bx::gfx_buffer_usage b) noexcept
{
	return static_cast<bx::gfx_buffer_usage>(static_cast<u32>(a) | static_cast<u32>(b));
}

constexpr bx::gfx_buffer_usage operator&(bx::gfx_buffer_usage a, bx::gfx_buffer_usage b) noexcept
{
	return static_cast<bx::gfx_buffer_usage>(static_cast<u32>(a) & static_cast<u32>(b));
}

constexpr bool bx_enum_any(bx::gfx_buffer_usage a) noexcept
{
	return static_cast<u32>(a) != 0;
}

#endif //BXL_HPP
