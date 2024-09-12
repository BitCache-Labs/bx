#pragma once

#include "bx/engine/core/ecs.hpp"

#include "bx/engine/core/serial.serial.hpp"
#include "bx/engine/containers/list.serial.hpp"

template <>
class Serial<Entity>
{
public:
	template <class Archive>
	static void Save(Archive& ar, const Entity& data)
	{
		ar(cereal::make_nvp("id", data.m_id));
	}

	template <class Archive>
	static void Load(Archive& ar, Entity& data)
	{
		ar(cereal::make_nvp("id", data.m_id));
	}
};
REGISTER_SERIAL(Entity);