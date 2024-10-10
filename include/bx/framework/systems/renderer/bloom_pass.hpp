#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class BloomPass : NoCopy
{
public:
	BloomPass(u32 width, u32 height);
	~BloomPass();

	void Dispatch(const Camera& camera, TextureHandle colorTarget);

	static void ClearPipelineCache();

	f32 intensity = 0.5;

private:
	u32 width, height;
	u32 mipCount;

	BufferHandle constantBuffer;
	BufferHandle resolveConstantBuffer;
	TextureHandle mippedColorTarget;
};