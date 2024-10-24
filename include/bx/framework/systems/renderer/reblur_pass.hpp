#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/systems/renderer/anti_firefly_pass.hpp"

struct ReblurDispatchInfo
{
	TextureHandle unresolvedIllumination;

	TextureViewHandle gbufferView;
	TextureViewHandle gbufferHistoryView;
	TextureViewHandle neGbufferHistoryView;
	TextureViewHandle reprojectionView;
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

	u32 spatialFilterSteps = 4; // Odd numbers are slower!

private:
	u32 width, height;
	u32 lightingWidth, lightingHeight;
	u32 frameIdx;

	std::unique_ptr<AntiFireflyPass> antiFireflyPass;

	SamplerHandle nearestClampSampler;
	SamplerHandle linearClampSampler;

	TextureHandle tmpIlluminationTexture;
	TextureViewHandle tmpIlluminationTextureView;
	TextureHandle historyTexture[2];
	TextureViewHandle historyTextureView[2];
	TextureHandle varianceTexture[2];
	TextureViewHandle varianceTextureView[2];

	BufferHandle temporalAccumConstantsBuffer;
	BufferHandle aTrousConstantsBuffer;

	BindGroupHandle CreateTemporalAccumBindGroup(const ReblurDispatchInfo& dispatchInfo) const;
	BindGroupHandle CreateATrousBindGroup(const ReblurDispatchInfo& dispatchInfo, bool pingPong) const;
};