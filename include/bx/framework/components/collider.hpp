#pragma once

#include "bx/framework/resources/mesh.hpp"

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/math.hpp>
#include <bx/engine/core/resource.hpp>
#include <bx/engine/modules/physics.hpp>

class Collider : public Component<Collider>
{
public:
	Collider();

	void OnPostCopy() override
	{
		m_isDirty = true;
		m_collider = PHYSICS_INVALID_HANDLE;
	}

	void OnRemoved() override
	{
		if (m_collider != PHYSICS_INVALID_HANDLE)
			Physics::DestroyCollider(m_collider);
	}

	inline ColliderShape GetShape() const { return m_shape; }
	inline void SetShape(ColliderShape shape) { m_shape = shape; m_isDirty = true; }

	inline Vec3 GetCenter() const { return m_center; }
	inline void SetCenter(const Vec3& center) { m_center = center; m_isDirty = true; }

	inline Vec3 GetSize() const { return m_size; }
	inline void SetSize(const Vec3& size) { m_size = size; m_isDirty = true; }

	inline f32 GetRadius() const { return m_radius; }
	inline void SetRadius(f32 radius) { m_radius = radius; m_isDirty = true; }

	inline f32 GetHeight() const { return m_height; }
	inline void SetHeight(f32 height) { m_height = height; m_isDirty = true; }

	inline ColliderAxis GetAxis() const { return m_axis; }
	inline void SetAxis(ColliderAxis axis) { m_axis = axis; m_isDirty = true; }

	inline const Resource<Mesh>& GetMesh() const { return m_mesh; }
	inline void SetMesh(const Resource<Mesh>& mesh) { m_mesh = mesh; m_isDirty = true; }

	inline PhysicsHandle GetCollider() const { return m_collider; }

	inline void Build(bool isObject, const Mat4& matrix)
	{
		if (!m_isDirty)
			return;

		m_isDirty = false;

		if (m_collider != PHYSICS_INVALID_HANDLE)
			Physics::DestroyCollider(m_collider);

		ColliderInfo info;
		info.matrix = matrix;
		info.isObject = isObject;
		info.shape = m_shape;
		info.center = m_center;
		info.size = m_size;
		info.radius = m_radius;
		info.height = m_height;
		info.axis = m_axis;
		info.scale = m_scale;
		info.isConcave = m_isConcave;
		info.id = GetEntity().GetId();
		if (m_shape == ColliderShape::MESH && m_mesh.IsValid())
		{
			const auto& meshData = m_mesh.GetData();
			ComputeColliderVertices(meshData.GetVertices(), meshData.GetIndices(), info.vertices);
		}
		m_collider = Physics::CreateCollider(info);
	}

private:
	void ComputeColliderVertices(const List<Vec3>& inVertices, const List<u32>& inTriangles, List<Vec3>& outVertices) const;

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Inspector;

	bool m_isDirty = true;

	ColliderShape m_shape = ColliderShape::BOX;

	Vec3 m_center = Vec3(0, 0, 0);
	Vec3 m_size = Vec3(1, 1, 1);
	f32 m_radius = 1;
	f32 m_height = 1;
	ColliderAxis m_axis = ColliderAxis::AXIS_Y;
	Vec3 m_scale = Vec3(1, 1, 1);
	bool m_isConcave = false;
	Resource<Mesh> m_mesh;

	PhysicsHandle m_collider = PHYSICS_INVALID_HANDLE;
};