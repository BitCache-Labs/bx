#pragma once

#include <bx/engine/core/math.hpp>
#include <bx/engine/modules/graphics.hpp>

class Texture
{
public:
	i32 channels = 0;
	i32 width = 0;
	i32 height = 0;
	i32 depth = 0;
	List<u8> pixels;

	inline TextureHandle GetTexture() const { return m_texture; }

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Resource;

	TextureHandle m_texture = TextureHandle::null;
};