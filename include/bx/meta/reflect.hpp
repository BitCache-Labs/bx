#pragma once

#include <bx/containers/hash_map.hpp>
#include <bx/meta/type.hpp>

class ReflectManager
{
public:
	static void Register(const MetaType& type)
	{
		GetTypeMap().insert(std::make_pair(type.GetTypeId(), type));
	}

	static const MetaType& GetType(TypeId typeId)
	{
		auto it = GetTypeMap().find(typeId);
		BX_ENSURE(it != GetTypeMap().end());
		return it->second;
	}

private:
	static HashMap<TypeId, MetaType>& GetTypeMap()
	{
		static HashMap<u32, MetaType> g_typeMap;
		return g_typeMap;
	}
};

template<typename T>
struct Reflect
{
	Reflect() { static_assert(false, "No specialization for reflect."); }
};

#define REFLECT_AT_START_UP(Type) static volatile const Reflect<Type> g_reflected##Type{};