#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

struct ReblurDispatchInfo
{
	TextureHandle unresolvedIllumination;

	TextureViewHandle gbufferView;
	TextureViewHandle gbufferHistoryView;
	TextureViewHandle velocityView;
};

class ReblurPass : NoCopy
{
public:
	ReblurPass(u32 width, u32 height);
	~ReblurPass();

	void Dispatch(const ReblurDispatchInfo& dispatchInfo);

	static void ClearPipelineCache();

	u32 seed = 1337;

private:
	u32 width, height;
	u32 frameIdx;

	TextureHandle tmpIlluminationTexture;
	TextureViewHandle tmpIlluminationTextureView;
	TextureHandle historyTexture[2];
	TextureViewHandle historyTextureView[2];

	BufferHandle preBlurConstantsBuffer;
	BufferHandle temporalAccumConstantsBuffer;

	BindGroupHandle CreatePreBlurBindGroup(const ReblurDispatchInfo& dispatchInfo) const;
	BindGroupHandle CreateTemporalAccumBindGroup(const ReblurDispatchInfo& dispatchInfo) const;
};