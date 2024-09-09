#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class GBufferPass : NoCopy
{
public:
	GBufferPass(TextureHandle colorTarget, TextureHandle depthTarget);
	~GBufferPass();

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