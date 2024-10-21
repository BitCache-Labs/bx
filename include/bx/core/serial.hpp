#pragma once

#include <bx/bx.hpp>
#include <bx/containers/string.hpp>

#include <rttr/type>

class BX_API Serializer
{
public:
	static String Save(rttr::instance obj);
	static rttr::instance Load(const String& str);
};