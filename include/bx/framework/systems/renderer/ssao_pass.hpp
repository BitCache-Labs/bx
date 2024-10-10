#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class SsaoPass : NoCopy
{
public:
	SsaoPass(u32 width, u32 height);
	~SsaoPass();

	void Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle neGbufferView, TextureViewHandle ambientEmissiveBaseColorView);

	static void ClearPipelineCache();

	u32 sampleCount = 16;
	b8 reducedBias = true;
	u32 seed = 1337;
	f32 intensity = 1.0;
	f32 depthOffset = 0.0;
	f32 radius = 1.4;

private:
	u32 width, height;

	BufferHandle constantBuffer;
};