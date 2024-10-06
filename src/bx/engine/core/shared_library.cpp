#include "bx/engine/core/shared_library.hpp"

#ifdef BX_PLATFORM_PC

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "bx/engine/core/file.hpp"

SharedLibrary::SharedLibrary(const String& path)
	: path(path)
{
	String resolvedPath = File::RemoveExt(path) + ".dll";
	HINSTANCE library = LoadLibrary(resolvedPath.c_str());
	BX_ENSURE(library);
	handle = reinterpret_cast<void*>(library);
}

SharedLibrary::~SharedLibrary()
{
	FreeLibrary(reinterpret_cast<HINSTANCE>(handle));
}

void* SharedLibrary::LoadRawFunction(const String& name)
{
	// TODO: fix this bs
	//HINSTANCE library = reinterpret_cast<HINSTANCE>(handle);

	String resolvedPath = File::RemoveExt(path) + ".dll";
	HINSTANCE library = LoadLibrary(resolvedPath.c_str());
	BX_ENSURE(library);
	return (void*)GetProcAddress(library, name.c_str());
}

#elif defined (BX_PLATFORM_LINUX)

#include <dlfcn.h>

SharedLibrary::SharedLibrary(const String& path)
{
	String resolvedPath = File::RemoveExt(path) + ".so";
	void* library = dlopen(resolvedPath.c_str(), RTLD_NOW | RTLD_LOCAL);
	BX_ENSURE(library);
	handle = library;
}

SharedLibrary::~SharedLibrary()
{

}

void* SharedLibrary::LoadRawFunction(const String& name)
{
	return (void*)dlsym(handle, name.c_str());
}

#endif