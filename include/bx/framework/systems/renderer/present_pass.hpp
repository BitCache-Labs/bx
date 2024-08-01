#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

// Draw a texture to the screen using a fullscreen triangle.
// Texture must be in ACEScg space and will be converted to the appropriated color space based on the output window.
class PresentPass : NoCopy
{
public:
	PresentPass(TextureHandle texture);
	~PresentPass();

	void Dispatch();

	static void ClearPipelineCache();

private:
	BindGroupHandle bindGroup;
	TextureViewHandle textureView;
	u32 width, height;
};