#pragma once

#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/type.hpp"

class SharedLibrary : NoCopy
{
public:
	SharedLibrary(const String& path);
	~SharedLibrary();

	void* LoadRawFunction(const String& name);

	template <typename T>
	T LoadFunction(const String& name)
	{
		return reinterpret_cast<T>(LoadRawFunction(name));
	}

private:
	const String path;

	void* handle;
};