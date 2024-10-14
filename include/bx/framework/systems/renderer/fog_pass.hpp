#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class FogPass : NoCopy
{
public:
	FogPass(u32 width, u32 height);
	~FogPass();

	void Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle ambientEmissiveBaseColorView, TextureViewHandle throughputView);

	static void ClearPipelineCache();

	Vec3 fogColor = Vec3(0.1, 0.1, 0.1);
	f32 emissiveBias = 0.1;

private:
	u32 width, height;

	BufferHandle constantBuffer;
};