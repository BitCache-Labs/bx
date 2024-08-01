#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class SrgbToLinearPass : NoCopy
{
public:
	SrgbToLinearPass(TextureHandle srgbTexture, TextureHandle linearTexture);
	~SrgbToLinearPass();

	void Dispatch();

	static void ClearPipelineCache();

private:
	BindGroupHandle bindGroup;
	TextureViewHandle srgbTextureView;
	TextureViewHandle linearTextureView;
	u32 width, height;
};