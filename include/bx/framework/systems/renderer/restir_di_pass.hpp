#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class Restir : NoCopy
{
public:
	struct RestirSample
	{
		Vec4 x0;
		Vec4 x1;
		Vec4 x2;
		//f32 weight;
		//f32 unoccludedContributionWeight;
		//u32 _PADDING0;
		//u32 _PADDING1;
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