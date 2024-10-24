#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class ColorCorrectPass : NoCopy
{
public:
	ColorCorrectPass(TextureHandle colorTarget);
	~ColorCorrectPass();

	void Dispatch();

	static void ClearPipelineCache();

private:
	u32 width, height;

	BindGroupHandle bindGroup;
	BufferHandle constantBuffer;
	TextureViewHandle colorTargetView;
};