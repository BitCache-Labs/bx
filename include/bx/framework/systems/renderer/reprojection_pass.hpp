#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class ReprojectionPass : NoCopy
{
public:
	ReprojectionPass(u32 width, u32 height);
	~ReprojectionPass();

	TextureHandle GetReprojection() const;
	TextureViewHandle GetReprojectionView() const;

	void Dispatch(const Camera& camera, TextureViewHandle depthView, TextureViewHandle velocityView);

	static void ClearPipelineCache();

private:
	u32 width, height;

	BufferHandle constantBuffer;
	TextureHandle reprojectionTexture;
	TextureViewHandle reprojectionTextureView;
	SamplerHandle nearestClampSampler;
};