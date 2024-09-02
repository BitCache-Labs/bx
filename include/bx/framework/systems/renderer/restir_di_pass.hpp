#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class Restir : NoCopy
{
public:
	struct RestirSample
	{
		Vec3 x0;
		f32 weight;
		Vec3 x1;
		f32 unoccludedContributionWeight;
		Vec3 x2;
		u32 _PADDING0;
	};

public:
	static BindGroupLayoutDescriptor GetBindGroupLayout();
	constexpr static u32 BIND_GROUP_SET = 4;
};

class RestirDiPass : NoCopy
{
public:
	RestirDiPass(u32 width, u32 height);
	~RestirDiPass();

	BindGroupHandle CreateBindGroup(ComputePipelineHandle pipeline, b8 flipRestirSamples) const;

	void Dispatch();

	static void ClearPipelineCache();

	u32 seed = 1337;
	u32 pixelRadius = 5;

private:
	u32 width, height;

	BufferHandle samplesBuffer;
	BufferHandle outSamplesBuffer;
	BufferHandle samplesHistoryBuffer;

	BufferHandle spatialReuseConstantsBuffer;

	BindGroupHandle spatialReuseBindGroup;
	BindGroupHandle restirBindGroup;
};