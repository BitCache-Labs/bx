#pragma once

#include "bx/engine/core/math.hpp"

#include <cereal/cereal.hpp>

namespace cereal
{
	template<class Archive>
	void serialize(Archive& archive, Vec2& data)
	{
		archive(make_nvp("x", data.x), make_nvp("y", data.y));
	}

	template<class Archive>
	void serialize(Archive& archive, Vec3& data)
	{
		archive(make_nvp("x", data.x), make_nvp("y", data.y), make_nvp("z", data.z));
	}

	template<class Archive>
	void serialize(Archive& archive, Vec4& data)
	{
		archive(make_nvp("x", data.x), make_nvp("y", data.y), make_nvp("z", data.z), make_nvp("w", data.w));
	}

	template<class Archive>
	void serialize(Archive& archive, Vec4i& data)
	{
		archive(make_nvp("x", data.x), make_nvp("y", data.y), make_nvp("z", data.z), make_nvp("w", data.w));
	}

	template<class Archive>
	void serialize(Archive& archive, Color& data)
	{
		archive(make_nvp("r", data.r), make_nvp("g", data.g), make_nvp("b", data.b), make_nvp("a", data.a));
	}

	template<class Archive>
	void serialize(Archive& archive, Quat& data)
	{
		archive(make_nvp("x", data.x), make_nvp("y", data.y), make_nvp("z", data.z), make_nvp("w", data.w));
	}

	template<class Archive>
	void serialize(Archive& archive, Mat4& data)
	{
		archive(make_nvp("x", data.basis[0]), make_nvp("y", data.basis[1]), make_nvp("z", data.basis[2]), make_nvp("w", data.basis[3]));
	}
}