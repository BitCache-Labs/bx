#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class AntiFireflyPass : NoCopy
{
public:
	AntiFireflyPass(u32 width, u32 height);
	~AntiFireflyPass();

	TextureHandle GetResolvedColorTarget() const;

	void Dispatch(TextureHandle colorTarget);

	static void ClearPipelineCache();

private:
	u32 width, height;

	SamplerHandle linearClampSampler;
	BufferHandle constantBuffer;

	TextureHandle resolvedColorTarget;
	TextureViewHandle resolvedColorTargetView;
};