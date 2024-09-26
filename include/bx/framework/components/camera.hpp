#pragma once

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/math.hpp>

class Camera : public Component<Camera>
{
public:
	Camera();

	inline const f32& GetFov() const { return m_fov; }
	inline void SetFov(f32 fov) { m_fov = fov; m_dirty = true; }

	inline const f32& GetAspect() const { return m_aspect; }
	inline void SetAspect(f32 aspect) { m_aspect = aspect; m_dirty = true; }

	inline const f32& GetZNear() const { return m_zNear; }
	inline void SetZNear(f32 zNear) { m_zNear = zNear; m_dirty = true; }

	inline const f32& GetZFar() const { return m_zFar; }
	inline void SetZFar(f32 zFar) { m_zFar = zFar; m_dirty = true; }

	inline const Mat4& GetProjection() const { return m_projection; }
	inline const Mat4& GetInvProjection() const { return m_invProjection; }
	inline const Mat4& GetPrevInvProjection() const { return m_prevInvProjection; }

	inline void SetProjection(const Mat4& projection)
	{
		m_projection = projection;
		m_invProjection = m_projection.Inverse();

		m_dirty = true;
	}

	inline const Mat4& GetView() const { return m_view; }
	inline const Mat4& GetInvView() const { return m_invView; }
	inline const Mat4& GetPrevInvView() const { return m_prevInvView; }

	inline void SetView(const Mat4& view)
	{
		m_view = view;
		m_prevInvView = m_invView;
		m_invView = m_view.Inverse();

		m_dirty = true;
	}

	inline const Mat4& GetViewProjection() const { return m_viewProj; }
	inline const Mat4& GetInvViewProjection() const { return m_invViewProj; }
	inline const Mat4& GetPrevViewProjection() const { return m_prevViewProj; }

	inline void Update()
	{
		if (!m_dirty)
			return;

		m_projection = Mat4::Perspective(m_fov, m_aspect, m_zNear, m_zFar);
		m_prevInvProjection = m_invProjection;
		m_invProjection = m_projection.Inverse();

		m_prevViewProj = m_viewProj;
		m_viewProj = m_projection * m_view;
		m_invViewProj = m_viewProj.Inverse();
	}

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Inspector;

	bool m_dirty = true;

	f32 m_fov = 60.0f;
	f32 m_aspect = 1.0f;
	f32 m_zNear = 0.01f;
	f32 m_zFar = 1000.0f;

	Mat4 m_view = Mat4::Identity();
	Mat4 m_invView = Mat4::Identity();
	Mat4 m_prevInvView = Mat4::Identity();

	Mat4 m_projection = Mat4::Identity();
	Mat4 m_invProjection = Mat4::Identity();
	Mat4 m_prevInvProjection = Mat4::Identity();

	Mat4 m_viewProj = Mat4::Identity();
	Mat4 m_invViewProj = Mat4::Identity();
	Mat4 m_prevViewProj = Mat4::Identity();
};