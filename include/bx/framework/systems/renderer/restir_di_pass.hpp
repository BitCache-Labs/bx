#pragma once

#include "bx/engine/core/guard.hpp"

#include "bx/engine/modules/graphics.hpp"

class Restir : NoCopy
{
public:
	struct RestirSample // TODO: proper layout?
	{
		Vec4 x0;
		Vec4 x1;
		Vec4 x2;
	};

	struct Reservoir
	{
		RestirSample sample;
		f32 weightSum;
		f32 weight;
		u32 sampleCount;
		u32 _PADDING0;
	};

public:
	static BindGroupLayoutDescriptor GetBindGroupLayout();
	constexpr static u32 BIND_GROUP_SET = 4;
};

class RestirDiPass : NoCopy
{
public:
	RestirDiPass(u32 width, u32 height, TlasHandle tlas, TextureViewHandle gbufferView);
	~RestirDiPass();

	BindGroupHandle CreateBindGroup(ComputePipelineHandle pipeline, b8 flipRestirSamples) const;

	void Dispatch();

	static void ClearPipelineCache();

	u32 seed = 1337;
	u32 pixelRadius = 30;

	static constexpr u32 SPATIAL_REUSE_PASSES = 1;

private:
	u32 width, height;

	BufferHandle samplesBuffer;
	BufferHandle outSamplesBuffer;
	BufferHandle samplesHistoryBuffer;

	BufferHandle spatialReuseConstantsBuffers[SPATIAL_REUSE_PASSES];
	BufferHandle temporalReuseConstantsBuffer;

	BindGroupHandle spatialReuseBindGroups[SPATIAL_REUSE_PASSES];
	BindGroupHandle temporalReuseBindGroup;
	BindGroupHandle restirTemporalBindGroup;
	BindGroupHandle restirSpatialBindGroups[SPATIAL_REUSE_PASSES];
};