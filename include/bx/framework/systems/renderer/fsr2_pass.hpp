#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

#include "bx/framework/components/camera.hpp"

#include <ffx/ffx_fsr2.h>

class Fsr2Pass : NoCopy
{
public:
	Fsr2Pass(u32 width, u32 height, u32 outputWidth, u32 outputHeight);
	~Fsr2Pass();

	TextureHandle GetResolvedColorTarget() const;

	void Dispatch(const Camera& camera, TextureHandle colorTarget, TextureHandle depthTarget, TextureHandle velocityTarget);
	void NextFrame();

	static void ClearPipelineCache();

private:
	u32 width, height;
	u32 outputWidth, outputHeight;

	TextureHandle outputTarget;

	FfxFsr2Context fsr2Context;
	void* scratchBuffer;
};