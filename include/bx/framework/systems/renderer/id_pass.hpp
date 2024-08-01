#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class IdPass : NoCopy
{
public:
	IdPass(TextureHandle colorTarget, TextureHandle depthTarget);
	~IdPass();

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