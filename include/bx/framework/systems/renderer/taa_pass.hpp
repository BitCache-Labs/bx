#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class TaaPass : NoCopy
{
public:
	TaaPass(u32 width, u32 height, u32 colorWidth, u32 colorHeight);
	~TaaPass();

	TextureViewHandle GetResolvedColorTargetView() const;
	TextureHandle GetResolvedColorTarget() const;

	void Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle gbufferHistoryView, TextureViewHandle velocityTargetView);
	void NextFrame();

	static void ClearPipelineCache();

	float historyWeight = 0.9;

private:
	BufferHandle constantBuffer;

	SamplerHandle linearClampSampler;
	SamplerHandle nearestClampSampler;

	TextureHandle resolvedColorTarget;
	TextureViewHandle resolvedColorTargetView;
	TextureHandle resolvedColorTargetHistory;
	TextureViewHandle resolvedColorTargetHistoryView;
	u32 width, height;
	u32 colorWidth, colorHeight;
};