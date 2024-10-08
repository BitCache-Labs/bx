#pragma once

#include <string>
#include <vector>

#include <bx/core/byte_types.hpp>
#include "bx/core/hash.hpp"
#include "bx/core/macros.hpp"
#include "bx/containers/string.hpp"
#include "bx/containers/list.hpp"
#include "bx/containers/hash_map.hpp"

#include "bx/meta/meta.hpp"

using TypeId = u32;
constexpr TypeId INVALID_TYPEID = -1;

template <typename TType>
class TypeImpl
{
private:
	template <typename T>
	friend class Type;

	static TypeId Id()
	{
		static Hash<String> hashFn;
		static const TypeId id = hashFn(ClassName());
		return id;
	}

	static String ClassName()
	{
		return BX_FUNCTION;
		//return wnaabi::type_info<TType>::name_tokens(wnaabi::runtime_visitors::stringify_t{}).str;
	}
};

template <typename TType>
class Type
{
public:
	static TypeId Id()
	{
		return TypeImpl<meta::decay_t<TType>>::Id();
	}

	static String ClassName()
	{
		return TypeImpl<meta::decay_t<TType>>::ClassName();
	}
};