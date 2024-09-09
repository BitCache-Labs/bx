#pragma once

#include "bx/framework/resources/mesh.hpp"

#include <bx/engine/core/ecs.hpp>
#include <bx/engine/core/resource.hpp>
#include <bx/engine/containers/list.hpp>

class MeshFilter : public Component<MeshFilter>
{
public:
	MeshFilter();

	inline const List<Resource<Mesh>>& GetMeshes() const { return m_meshes; }
	inline SizeType GetMeshCount() const { return m_meshes.size(); }
	
	inline void AddMesh(const Resource<Mesh>& mesh)
	{
		m_meshes.emplace_back(mesh);
	}

	inline const Resource<Mesh>& GetMesh(SizeType index) const
	{
		BX_ENSURE(index < m_meshes.size());
		return m_meshes[index];
	}
	
	inline void SetMesh(SizeType index, const Resource<Mesh>& mesh)
	{
		BX_ENSURE(index < m_meshes.size());
		m_meshes[index] = mesh;
	}

	inline void RemoveMesh(SizeType index)
	{
		BX_ENSURE(index < m_meshes.size());
		m_meshes.erase(m_meshes.begin() + index);
	}

	List<u32> m_blasInstanceIndices;

private:
	template <typename T>
	friend class Serial;

	template <typename T>
	friend class Inspector;

	List<Resource<Mesh>> m_meshes;
};