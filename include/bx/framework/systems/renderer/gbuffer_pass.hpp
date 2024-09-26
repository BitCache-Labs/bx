#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

// Renders multiple features of all objects to color target where
// r: 1 over square depth (can be used to reconstruct ws position)
// g: normal as PackedNormalizedXyz10
// b: texture coordinate as packHalf2x16
// a: first bit for front facing, remaining 31 bits for blas instance index

class GBufferPass : NoCopy
{
public:
	GBufferPass(TextureHandle depthTarget);
	~GBufferPass();

	TextureViewHandle GetColorTargetView() const;
	TextureViewHandle GetColorTargetHistoryView() const;
	TextureViewHandle GetVelocityTargetView() const;

	void Dispatch(const Camera& camera);
	void NextFrame();

	static void ClearPipelineCache();

private:
	BufferHandle constantBuffer;

	TextureHandle colorTarget[2];
	TextureHandle velocityTarget;
	TextureHandle depthTarget;
	TextureViewHandle colorTargetView[2];
	TextureViewHandle velocityTargetView;
	TextureViewHandle depthTargetView;
	u32 width, height;
	u32 frameIdx;
};