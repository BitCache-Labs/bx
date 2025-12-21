#ifndef BX_APP_IMPL
#define BX_APP_IMPL

#include <bx/app.hpp>
#include <unordered_map>

namespace bx
{
	struct bx_api handle_t
	{
		handle_t() = default;

		explicit handle_t(const handle_id value)
			: id(value)
		{}

		handle_t(const u32 data, const u32 meta)
			: data(data)
			, meta(meta)
		{}

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

	template <typename T>
	struct bx_api handlemap
	{
		inline handle_id insert(const T& obj) bx_noexcept
		{
			auto handle = counter++;
			map.insert(std::make_pair(handle, obj));
			return handle;
		}

		inline void remove(handle_id handle) bx_noexcept
		{
			const auto it = map.find(handle);
			if (it == map.end())
				return;

			map.erase(it);
		}

		inline T* get(handle_id handle) bx_noexcept
		{
			auto it = map.find(handle);
			if (it == map.end())
				return nullptr;
			return &it->second;
		}

	private:
		handle_id counter{ 1 };
		std::unordered_map<handle_id, T> map;
	};

	bx_api bool dvc_init(const app_config_t& config) bx_noexcept;
	bx_api void dvc_shutdown() bx_noexcept;

	bx_api bool gfx_init(const app_config_t& config) bx_noexcept;
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

#endif // BX_APP_IMPL
