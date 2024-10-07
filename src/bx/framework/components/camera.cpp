#include "bx/framework/components/camera.hpp"
#include "bx/framework/components/camera.serial.hpp"

u32 GetJitterPhaseCount(u32 renderWidth, u32 displayWidth)
{
	float basePhaseCount = 8.0f;
	u32 jitterPhaseCount = u32(basePhaseCount * pow((f32(displayWidth) / renderWidth), 2.0));
	return jitterPhaseCount;
}

Vec2 GetJitterOffset(u32 index, u32 phaseCount)
{
	return Vec2(
		Math::Halton((index % phaseCount) + 1, 2) - 0.5f,
		Math::Halton((index % phaseCount) + 1, 3) - 0.5f
	);
}

Camera::Camera()
{
	// Dummy so compiler doesn't optimize away this source file
}

void Camera::Update(u32 renderWidth, u32 displayWidth)
{
	if (!m_dirty)
		return;

	m_projection = Mat4::Perspective(m_fov, m_aspect, m_zNear, m_zFar);
	m_prevInvProjection = m_invProjection;
	m_invProjection = m_projection.Inverse();

	m_prevViewProj = m_viewProj;
	m_viewProj = m_projection * m_view;
	m_invViewProj = m_viewProj.Inverse();

	m_jitter = GetJitterOffset(frameIdx, GetJitterPhaseCount(renderWidth, displayWidth));
	frameIdx++;
}