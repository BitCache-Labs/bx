#pragma once

#include <bx/engine/core/math.hpp>
#include <bx/engine/containers/list.hpp>
#include <bx/engine/modules/graphics.hpp>

class Mesh
{
public:
	struct Vertex
	{
		Vec3 position;
		Vec4 color;
		Vec3 normal;
		Vec3 tangent;
		Vec2 uv;
		Vec4i bones;
		Vec4 weights;
	};

public:
	Mesh() {}
	Mesh(
		const Mat4& transform,
		const List<Vec3>& vertices,
		const List<Vec4>& colors,
		const List<Vec3>& normals,
		const List<Vec3>& tangents,
		const List<Vec2>& uvs,
		const List<Vec4i>& bones,
		const List<Vec4>& weights,
		const List<u32>& indices)
		: m_transform(transform)
		, m_vertices(vertices)
		, m_colors(colors)
		, m_normals(normals)
		, m_tangents(tangents)
		, m_uvs(uvs)
		, m_bones(bones)
		, m_weights(weights)
		, m_indices(indices)
	{}

	inline const Mat4& GetMatrix() const { return m_transform; }
	inline void SetMatrix(const Mat4& matrix) { m_transform = matrix; }

	inline const List<Vec3>& GetVertices() const { return m_vertices; }
	inline void SetVertices(const List<Vec3>& vertices) { m_vertices = vertices; }

	inline const List<Vec4>& GetColors() const { return m_colors; }
	inline void SetColors(const List<Vec4>& colors) { m_colors = colors; }

	inline const List<Vec3>& GetNormals() const { return m_normals; }
	inline void SetNormals(const List<Vec3>& normals) { m_normals = normals; }

	inline const List<Vec3>& GetTangents() const { return m_tangents; }
	inline void SetTangents(const List<Vec3>& tangents) { m_tangents = tangents; }

	inline const List<Vec2>& GetUvs() const { return m_uvs; }
	inline void SetUvs(const List<Vec2>& uvs) { m_uvs = uvs; }

	inline const List<Vec4i>& GetBones() const { return m_bones; }
	inline void SetBoneIds(const List<Vec4i>& bones) { m_bones = bones; }

	inline const List<Vec4>& GetWeights() const { return m_weights; }
	inline void SetWeights(const List<Vec4>& weights) { m_weights = weights; }

	inline const List<u32>& GetIndices() const { return m_indices; }
	inline void SetIndices(const List<u32>& indices) { m_indices = indices; }

	inline BufferHandle GetVertexBuffer() const { return m_vertexBuffer; }
	inline BufferHandle GetIndexBuffer() const { return m_indexBuffer; }

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Resource;

	Mat4 m_transform;
	List<Vec3> m_vertices;
	List<Vec4> m_colors;
	List<Vec3> m_normals;
	List<Vec3> m_tangents;
	List<Vec2> m_uvs;
	List<Vec4i> m_bones;
	List<Vec4> m_weights;
	List<u32> m_indices;

	BufferHandle m_vertexBuffer = BufferHandle::null;
	BufferHandle m_indexBuffer = BufferHandle::null;
};