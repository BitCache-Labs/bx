#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

// Renders multiple features of all objects to color target where
// r: 1 over square depth
// g: normal as PackedNormalizedXyz10
// b: texture coordinate as packHalf2x16
// a: blas instance index

class GBufferPass : NoCopy
{
public:
	GBufferPass(TextureHandle depthTarget);
	~GBufferPass();

	TextureViewHandle GetColorTargetView() const;

	void Dispatch(const Camera& camera);

	static void ClearPipelineCache();

private:
	BufferHandle constantBuffer;

	TextureHandle colorTarget;
	TextureHandle depthTarget;
	TextureViewHandle colorTargetView;
	TextureViewHandle depthTargetView;
	u32 width, height;
};