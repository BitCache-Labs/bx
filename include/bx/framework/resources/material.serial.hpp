#pragma once

#include "bx/framework/resources/material.hpp"

#include "bx/framework/resources/shader.serial.hpp"
#include "bx/framework/resources/texture.serial.hpp"

#include <bx/engine/core/serial.serial.hpp>
#include <bx/engine/core/math.serial.hpp>
#include <bx/engine/core/resource.serial.hpp>
#include <bx/engine/containers/string.serial.hpp>
#include <bx/engine/containers/list.serial.hpp>
#include <bx/engine/containers/hash_map.serial.hpp>
#include <bx/engine/containers/tree.serial.hpp>

template <>
class Serial<Material>
{
public:
	template<class Archive>
	static void Save(Archive& ar, const Material& data)
	{
		ar(cereal::make_nvp("shader", data.m_shader));
		ar(cereal::make_nvp("textures", data.m_textures));
		ar(cereal::make_nvp("isEmissive", data.m_isEmissive));
	}

	template<class Archive>
	static void Load(Archive& ar, Material& data)
	{
		ar(cereal::make_nvp("shader", data.m_shader));
		ar(cereal::make_nvp("textures", data.m_textures));
		ar(cereal::make_nvp("isEmissive", data.m_isEmissive));
	}
};
REGISTER_SERIAL(Material);