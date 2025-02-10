#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/string.hpp>

class BX_API Serial
{
	BX_TYPE(Serial)

public:
	template <typename T>
	static bool Save(const T& obj, StringView filename)
	{
		return Save(rttr::variant(obj), filename);
	}

	template <typename T>
	static bool Load(T& obj, StringView filename)
	{
		return Load(rttr::variant(obj), filename);
	}

private:
	static bool Save(rttr::variant obj, StringView filename);
	static bool Load(rttr::variant obj, StringView filename);
};