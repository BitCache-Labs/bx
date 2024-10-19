#pragma once

#include <bx/engine/core/math.hpp>
#include <bx/engine/modules/graphics.hpp>

class Texture
{
public:
	u32 width = 0;
	u32 height = 0;
	u32 depth = 0;
	List<u8> pixels;
	TextureFormat format;

	inline TextureHandle GetTexture() const { return m_texture; }

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Resource;

	TextureHandle m_texture = TextureHandle::null;
};