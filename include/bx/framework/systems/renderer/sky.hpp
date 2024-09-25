#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/math.hpp"
#include "bx/engine/core/resource.hpp"

#include "bx/engine/modules/graphics.hpp"

struct SunInfo
{
	Vec3 direction = Vec3(-0.3, -1.0, 0.0);
	f32 size = 0.15;
	Color color = Color::White();
	f32 intensity = 15.0;
};

class Sky : NoCopy
{
public:
	Sky();
	~Sky();

	void Submit();

	BindGroupHandle CreateBindGroup(ComputePipelineHandle pipeline) const;

	static BindGroupLayoutDescriptor GetBindGroupLayout();
	constexpr static u32 BIND_GROUP_SET = 3;

	SunInfo sunInfo{};

private:
	BufferHandle skyConstantsBuffer = BufferHandle::null;
};