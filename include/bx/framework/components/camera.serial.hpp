#pragma once

#include "bx/framework/components/camera.hpp"

#include <bx/engine/core/serial.serial.hpp>

template <>
struct Serial<Camera>
{
	template <class Archive>
	static void Save(Archive& ar, Camera const& data)
	{
		ar(cereal::make_nvp("fov", data.m_fov));
		ar(cereal::make_nvp("aspect", data.m_aspect));
		ar(cereal::make_nvp("zNear", data.m_zNear));
		ar(cereal::make_nvp("zFar", data.m_zFar));
	}

	template <class Archive>
	static void Load(Archive& ar, Camera& data)
	{
		ar(cereal::make_nvp("fov", data.m_fov));
		ar(cereal::make_nvp("aspect", data.m_aspect));
		ar(cereal::make_nvp("zNear", data.m_zNear));
		ar(cereal::make_nvp("zFar", data.m_zFar));

		data.Update(1920, 1920);
	}
};

REGISTER_POLYMORPHIC_SERIAL(ComponentBase, Camera)