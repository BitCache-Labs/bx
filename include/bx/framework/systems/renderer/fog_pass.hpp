#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class FogPass : NoCopy
{
public:
	FogPass(u32 width, u32 height);
	~FogPass();

	void Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle ambientEmissiveBaseColorView);

	static void ClearPipelineCache();

private:
	u32 width, height;

	BufferHandle constantBuffer;
};