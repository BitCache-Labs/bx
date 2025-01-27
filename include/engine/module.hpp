#pragma once

#include <engine/type.hpp>

#define BX_MODULE(Name, ...)								\
	BX_TYPE(Name, __VA_ARGS__)								\
	BX_SINGLETON(Name)										\
	public:													\
	static Name& Get() noexcept;

#define BX_MODULE_INTERFACE(Name, ...)						\
	BX_TYPE(Name, __VA_ARGS__)								\
	BX_INTERFACE(Name)										\
    BX_NOCOPY(Name)											\
	public:													\
	static Name& Get() noexcept;

#define BX_MODULE_DEFINE(Name)								\
	Name& Name::Get() noexcept								\
	{														\
		static Name instance;								\
		return instance;									\
	}

#define BX_MODULE_DEFINE_INTERFACE(Name, Derived)			\
	Name& Name::Get() noexcept								\
	{														\
		return Derived::Get();								\
	}