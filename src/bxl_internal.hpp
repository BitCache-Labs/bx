#ifndef BXL_INTERNAL_HPP
#define BXL_INTERNAL_HPP

#include <bxl.hpp>
#include <unordered_map>

#ifdef BXL_GFX_VULKAN
#include <vulkan/vulkan.h>
#else
#ifdef __arm__
#define BXL_GFX_OPENGLES
#endif
#endif

bx_register_category(bxl)

namespace bxl
{
	struct handle_t
	{
		handle_t() = default;

		explicit handle_t(const bx::handle_id value)
			: id(value)
		{}

		handle_t(const u32 data, const u32 meta)
			: data(data)
			, meta(meta)
		{}

		explicit operator bx::handle_id() const { return id; }

		template<typename T>
		T get_data() const { return static_cast<T>(data); }

		template<typename T>
		T get_meta() const { return static_cast<T>(meta); }

		union
		{
			bx::handle_id id{};

			struct
			{
				u32 data;
				u32 meta;
			};
		};
	};

	template <typename T>
	struct bx_api handlemap
	{
		inline bx::handle_id insert(const T& obj) bx_noexcept
		{
			auto handle = counter++;
			map.insert(std::make_pair(handle, obj));
			return handle;
		}

		inline void remove(bx::handle_id handle) bx_noexcept
		{
			const auto it = map.find(handle);
			if (it == map.end())
				return;

			T::on_remove(it->second);
			map.erase(it);
		}

		inline T* get(bx::handle_id handle) bx_noexcept
		{
			auto it = map.find(handle);
			if (it == map.end())
				return nullptr;
			return &it->second;
		}

	private:
		bx::handle_id counter{ 1 };
		std::unordered_map<bx::handle_id, T> map;
	};

	bx_api bool device_init(const bx::app_config_t& config) bx_noexcept;
	bx_api void device_shutdown() bx_noexcept;

	bx_api bool gfx_init(const bx::app_config_t& config) bx_noexcept;
	bx_api void gfx_shutdown() bx_noexcept;
}

// Enum helpers
constexpr bx::gfx_shader_stage_t operator|(bx::gfx_shader_stage_t a, bx::gfx_shader_stage_t b) noexcept
{
	return static_cast<bx::gfx_shader_stage_t>(static_cast<u32>(a) | static_cast<u32>(b));
}

constexpr bx::gfx_shader_stage_t operator&(bx::gfx_shader_stage_t a, bx::gfx_shader_stage_t b) noexcept
{
	return static_cast<bx::gfx_shader_stage_t>(static_cast<u32>(a) & static_cast<u32>(b));
}

constexpr bool bx_enum_any(bx::gfx_shader_stage_t a) noexcept
{
	return static_cast<u32>(a) != 0;
}

constexpr bx::gfx_buffer_usage_t operator|(bx::gfx_buffer_usage_t a, bx::gfx_buffer_usage_t b) noexcept
{
	return static_cast<bx::gfx_buffer_usage_t>(static_cast<u32>(a) | static_cast<u32>(b));
}

constexpr bx::gfx_buffer_usage_t operator&(bx::gfx_buffer_usage_t a, bx::gfx_buffer_usage_t b) noexcept
{
	return static_cast<bx::gfx_buffer_usage_t>(static_cast<u32>(a) & static_cast<u32>(b));
}

constexpr bool bx_enum_any(bx::gfx_buffer_usage_t a) noexcept
{
	return static_cast<u32>(a) != 0;
}

#endif //BXL_INTERNAL_HPP
