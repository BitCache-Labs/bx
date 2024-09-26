#pragma once

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/math.hpp>
#include <bx/engine/containers/string.hpp>

class Transform : public Component<Transform>
{
public:
	Transform();

	inline const Vec3& GetPosition() const { return m_position; }
	inline void SetPosition(const Vec3& pos) { m_position = pos; m_isDirty = true; }

	inline const Quat& GetRotation() const { return m_rotation; }
	inline void SetRotation(const Quat& rot) { m_rotation = rot; m_isDirty = true; }

	inline const Vec3& GetScale() const { return m_scale; }
	inline void SetScale(const Vec3& scl) { m_scale = scl; m_isDirty = true; }

	inline void Set(const Vec3& pos, const Quat& rot, const Vec3& scl)
	{
		m_position = pos;
		m_rotation = rot;
		m_scale = scl;
		m_isDirty = true;
	}

	inline const Mat4& GetMatrix() const { return m_matrix; }
	inline const Mat4& GetInvMatrix() const { return m_invMatrix; }
	inline const Mat4& GetPrevMatrix() const { return m_prevMatrix; }

	inline void SetMatrix(const Mat4& matrix)
	{
		m_matrix = matrix;
		m_invMatrix = m_matrix.Inverse();

		Mat4::Decompose(m_matrix, m_position, m_rotation, m_scale);
	}

	inline bool WasDirty() const { return m_wasDirty; }

	inline void Update()
	{
		m_prevMatrix = m_matrix;

		m_wasDirty = false;
		if (!m_isDirty) return;
		m_isDirty = false;
		m_wasDirty = true;

		m_matrix = Mat4::TRS(m_position, m_rotation, m_scale);
		m_invMatrix = m_matrix.Inverse();
	}

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Inspector;

	bool m_isDirty = true;
	bool m_wasDirty = true;

	Vec3 m_position = Vec3(0, 0, 0);
	Quat m_rotation = Quat::Euler(0, 0, 0);
	Vec3 m_scale = Vec3(1, 1, 1);

	Mat4 m_matrix = Mat4::Identity();
	Mat4 m_invMatrix = Mat4::Identity();
	Mat4 m_prevMatrix = Mat4::Identity();
};