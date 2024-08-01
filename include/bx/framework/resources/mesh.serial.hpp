#pragma once

#include "bx/framework/resources/mesh.hpp"

#include <bx/engine/core/serial.serial.hpp>
#include <bx/engine/core/math.serial.hpp>
#include <bx/engine/core/resource.serial.hpp>
#include <bx/engine/containers/string.serial.hpp>
#include <bx/engine/containers/list.serial.hpp>
#include <bx/engine/containers/hash_map.serial.hpp>
#include <bx/engine/containers/tree.serial.hpp>

template <>
class Serial<Mesh>
{
public:
	template<class Archive>
	static void Save(Archive& ar, const Mesh& data)
	{
		ar(cereal::make_nvp("transform", data.m_transform));
		ar(cereal::make_nvp("vertices", data.m_vertices));
		ar(cereal::make_nvp("colors", data.m_colors));
		ar(cereal::make_nvp("normals", data.m_normals));
		ar(cereal::make_nvp("tangents", data.m_tangents));
		ar(cereal::make_nvp("uvs", data.m_uvs));
		ar(cereal::make_nvp("bones", data.m_bones));
		ar(cereal::make_nvp("weights", data.m_weights));
		ar(cereal::make_nvp("indices", data.m_indices));
	}

	template<class Archive>
	static void Load(Archive& ar, Mesh& data)
	{
		ar(cereal::make_nvp("transform", data.m_transform));
		ar(cereal::make_nvp("vertices", data.m_vertices));
		ar(cereal::make_nvp("colors", data.m_colors));
		ar(cereal::make_nvp("normals", data.m_normals));
		ar(cereal::make_nvp("tangents", data.m_tangents));
		ar(cereal::make_nvp("uvs", data.m_uvs));
		ar(cereal::make_nvp("bones", data.m_bones));
		ar(cereal::make_nvp("weights", data.m_weights));
		ar(cereal::make_nvp("indices", data.m_indices));
	}
};
REGISTER_SERIAL(Mesh);