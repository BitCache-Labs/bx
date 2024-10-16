#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

struct ReblurDispatchInfo
{
	TextureHandle unresolvedIllumination;

	TextureViewHandle gbufferView;
	TextureViewHandle gbufferHistoryView;
	TextureViewHandle neGbufferHistoryView;
	TextureViewHandle velocityView;
};

class ReblurPass : NoCopy
{
public:
	ReblurPass(u32 width, u32 height, u32 lightingWidth, u32 lightingHeight);
	~ReblurPass();

	void Dispatch(const ReblurDispatchInfo& dispatchInfo);

	static void ClearPipelineCache();

	u32 seed = 1337;
	b8 antiFirefly = true;

private:
	u32 width, height;
	u32 lightingWidth, lightingHeight;
	u32 frameIdx;

	SamplerHandle linearClampSampler;

	TextureHandle preBlurTexture;
	TextureViewHandle preBlurTextureView;
	TextureHandle tmpIlluminationTexture;
	TextureViewHandle tmpIlluminationTextureView;
	TextureHandle historyTexture[2];
	TextureViewHandle historyTextureView[2];

	BufferHandle preBlurConstantsBuffer;
	BufferHandle temporalAccumConstantsBuffer;
	BufferHandle historyFixConstantsBuffer;
	BufferHandle blurConstantsBuffer;

	BindGroupHandle CreatePreBlurBindGroup(const ReblurDispatchInfo& dispatchInfo) const;
	BindGroupHandle CreateTemporalAccumBindGroup(const ReblurDispatchInfo& dispatchInfo) const;
	BindGroupHandle CreateHistoryFixBindGroup(const ReblurDispatchInfo& dispatchInfo) const;
	BindGroupHandle CreateBlurBindGroup(const ReblurDispatchInfo& dispatchInfo) const;
};