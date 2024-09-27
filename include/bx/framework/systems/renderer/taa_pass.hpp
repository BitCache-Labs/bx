#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

class TaaPass : NoCopy
{
public:
	TaaPass(u32 width, u32 height);
	~TaaPass();

	TextureHandle GetResolvedColorTarget() const;

	void Dispatch(const Camera& camera, TextureHandle colorTarget, TextureViewHandle gbufferView, TextureViewHandle gbufferHistoryView, TextureViewHandle velocityTargetView);
	void NextFrame();

	static void ClearPipelineCache();

private:
	BufferHandle constantBuffer;

	TextureHandle resolvedColorTarget;
	TextureViewHandle resolvedColorTargetView;
	TextureHandle resolvedColorTargetHistory;
	TextureViewHandle resolvedColorTargetHistoryView;
	u32 width, height;
};