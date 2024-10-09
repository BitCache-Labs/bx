#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class SsaoPass : NoCopy
{
public:
	SsaoPass(u32 width, u32 height);
	~SsaoPass();

	void Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle ambientEmissiveBaseColorView);

	static void ClearPipelineCache();

	u32 sampleCount = 16;
	b8 reducedBias = true;
	u32 seed = 1337;

private:
	u32 width, height;

	BufferHandle constantBuffer;
};