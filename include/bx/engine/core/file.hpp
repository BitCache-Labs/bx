#pragma once

#include "bx/engine/core/byte_types.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/string.hpp"

#include <functional>

// TODO: Clean up the whole file handle way of working with files
// Would also be nice to implement a watch system to detect changes and re-load them.
// The above may tie in with the resource system.
// Idea: FileHandle may act as a wrapper around a file and a resource which depends on a file can check if it needs reloading.
// Since resources and objects are the go-to way of accesing data in this engine it would never require user code to get the latest data.
// Although changes would still need some sort of chain of events to update any creation made with them 
// (e.g. shaders, textures, etc on the graphics).

struct FileHandle
{
	String filepath;
	String filename;
	bool isDirectory = false;
};

using FindEachCallback = std::function<void(const String& path, const String& name)>;

class File
{
public:
	static void AddWildcard(const String& wildcard, const String& value);

	static bool Exists(const String& path);
	static u64 LastWrite(const String& filename);
	static String GetPath(const String& filename);

	static String GetExt(const String& filename);
	static String RemoveExt(const String& filename);

	static bool Move(const String& oldPath, const String& newPath);
	static bool Delete(const String& path);

	static bool CreateDirectory(const String& path);

	static bool ListFiles(const String& root, List<FileHandle>& files);
	static bool Find(const String& root, const String& filename, String& filepath);
	static void FindEach(const String& root, const String& ext, const FindEachCallback& callback);

	static List<char> ReadBinaryFile(const String& filename);
	static String ReadTextFile(const String& filename);
	static bool WriteTextFile(const String& filename, const String& text);

private:
	friend class Runtime;
	static void Initialize();
};