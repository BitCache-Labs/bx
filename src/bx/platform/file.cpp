#include "bx/engine/core/file.hpp"

#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/containers/hash_map.hpp"

#if defined(BX_PLATFORM_PC)
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")
#undef CreateDirectory
#ifdef UNICODE
#define WinCreateDirectory  CreateDirectoryW
#else
#define WinCreateDirectory  CreateDirectoryA
#endif // !UNICODE

#elif defined(BX_PLATFORM_LINUX)
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#endif

#include <cassert>
#include <fstream>
#include <sstream>

HashMap<String, String> s_wildcards;

static List<String> StringSplit(const String& source, const char* delimiter, bool keep_empty)
{
	List<String> results;

	size_t prev = 0;
	size_t next = 0;

	while ((next = source.find_first_of(delimiter, prev)) != String::npos)
	{
		if (keep_empty || (next - prev != 0))
		{
			results.emplace_back(source.substr(prev, next - prev));
		}
		prev = next + 1;
	}

	if (prev < source.size())
	{
		results.emplace_back(source.substr(prev));
	}

	return results;
}

void File::Initialize()
{
#if defined(BX_PLATFORM_PC) || defined(BX_PLATFORM_LINUX)
	AddWildcard("[assets]", BX_PROJECT_PATH"/game/assets");
	AddWildcard("[settings]", BX_PROJECT_PATH"/game/settings");

	// All platforms
	if (!Exists("[settings]/.ini"))
	{
		BX_LOGW("Not game .ini file, creating default.");
		File::WriteTextFile("[settings]/.ini", "GameName");
	}

	auto gameStr = ReadTextFile("[settings]/.ini");
	if (gameStr.empty())
	{
		gameStr = "GameName";
		BX_LOGW("Game .ini does not have game name, setting to default.");
		File::WriteTextFile("[settings]/.ini", gameStr);
	}
#endif

#if defined(BX_PLATFORM_PC)
	char* pValue;
	size_t len;
	_dupenv_s(&pValue, &len, "APPDATA");

	if (pValue != nullptr)
	{
		String save_path = String(pValue) + "/" + gameStr + "/";
		if (!Exists(save_path))
			CreateDirectory(save_path);

		AddWildcard("[save]", save_path);
	}

#elif defined(BX_PLATFORM_LINUX)
	const char* homeDir = std::getenv("HOME");
	if (homeDir)
	{
		String save_path = String(homeDir) + "/." + gameStr + "/";
		if (!Exists(save_path))
			CreateDirectory(save_path);

		AddWildcard("[save]", save_path);
	}
#endif

#ifdef BX_EDITOR_BUILD
	AddWildcard("[editor]", BX_PROJECT_PATH"/editor");
#endif
}

List<char> File::ReadBinaryFile(const String& filename)
{
	const auto path = GetPath(filename);
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		BX_LOGE("File {} with full path {} was not found!", filename, path);
		return List<char>();
	}

	const std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	
	List<char> buffer(size);
	if (file.read(buffer.data(), size))
		return  buffer;

	BX_ASSERT(false, "");
	return List<char>();
}

String File::ReadTextFile(const String& filename)
{
	const auto path = GetPath(filename);

	//std::ifstream file(path);
	//return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	std::ifstream file(path);
	if (!file.is_open())
	{
		BX_LOGE("File {} with full path {} was not found!", filename, path);
		return String();
	}

	file.seekg(0, std::ios::end);
	const size_t size = file.tellg();

	String buffer(size, '\0');
	file.seekg(0);
	file.read(&buffer[0], size);

	return buffer;
}

bool File::WriteTextFile(const String& filename, const String& text)
{
	auto fullpath = GetPath(filename);
	std::ofstream ofs;
	ofs.open(fullpath);

	if (ofs.is_open())
	{
		ofs << text;
		ofs.close();
		return true;
	}
	return false;
}

void File::AddWildcard(const String& wildcard, const String& value)
{
	if (!Exists(value))
	{
		if (!CreateDirectory(value))
		{
			BX_LOGE("Failed to create directory!");
			BX_ASSERT(false, "Create directory failed!");
		}
	}
	s_wildcards[wildcard] = value;
}

static String StringReplace(
	const String& subject,
	const String& search,
	const String& replace)
{
	String result(subject);
	size_t pos = 0;

	while ((pos = subject.find(search, pos)) != String::npos)
	{
		result.replace(pos, search.length(), replace);
		pos += search.length();
	}

	return result;
}

String File::GetPath(const String& filename)
{
	String filepath = filename;

	for (const auto& p : s_wildcards)
	{
		if (filepath.find(p.first) != String::npos)
			filepath = StringReplace(filepath, p.first, p.second);
	}

	return filepath;
}

String File::GetExt(const String& filename)
{
	return filename.substr(filename.find_last_of(".") + 1);
}

String File::RemoveExt(const String& filename)
{
	return filename.substr(0, filename.find_last_of("."));
}

bool File::Move(const String& oldPath, const String& newPath)
{
	String oldFullPath = File::GetPath(oldPath);
	String newFullPath = File::GetPath(newPath);

#if defined(BX_PLATFORM_PC)
	LPCSTR oldFileName = oldFullPath.c_str();
	LPCSTR newFileName = newFullPath.c_str();

	// Attempt to move the file with additional options
	if (MoveFileEx(oldFileName, newFileName, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
	{
		BX_LOGD("File moved old: ({}) -> new: ({})", oldPath, newPath);
		return true;
	}
	
	BX_LOGW("Failed to move file: {}", GetLastError());
	return false;

#elif defined(BX_PLATFORM_LINUX)
	BX_ASSERT(false, "Delete file not supported!");
	return false;

#else
	BX_ASSERT(false, "Delete file not supported!");
	return false;

#endif
}

bool File::Delete(const String& filename)
{
#if defined(BX_PLATFORM_PC)
	const String fullpath = GetPath(filename);
	
	DWORD attributes = GetFileAttributes(fullpath.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		BX_LOGD("Invalid path or unable to get attributes. Error code: {}", GetLastError());
		return false;
	}

	if (attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (RemoveDirectory(fullpath.c_str()))
		{
			BX_LOGD("Folder deleted: {}", filename);
		}
	}
	else
	{
		if (DeleteFile(fullpath.c_str()))
		{
			BX_LOGD("File deleted: {}", filename);
			return true;
		}
	}

	BX_LOGW("Failed to delete file: {}", filename);
	return false;

#elif defined(BX_PLATFORM_LINUX)
	BX_ASSERT(false, "Delete file not supported!");
	return false;

#else
	BX_ASSERT(false, "Delete file not supported!");
	return false;

#endif
}

bool File::Exists(const String& path)
{
	const auto filepath = GetPath(path);
	struct stat info;
	return (stat(filepath.c_str(), &info) == 0);
}

bool File::CreateDirectory(const String& path)
{
#if defined(BX_PLATFORM_PC)
	BOOL ret = WinCreateDirectory(path.c_str(), NULL);
	switch (GetLastError())
	{
	case ERROR_ALREADY_EXISTS: BX_LOGE("Directory already exists, failed to create!"); break;
	case ERROR_PATH_NOT_FOUND: BX_LOGE("Directory path not found, failed to create!"); break;
	}
	return ret;

#elif defined(BX_PLATFORM_LINUX)
	int ret = mkdir(path.c_str(), 0755);
    if (ret == 0)
        return true;

	switch (errno)
	{
	case EEXIST: BX_LOGE("Directory already exists, failed to create!"); break;
	case ENOENT: BX_LOGE("Directory path not found, failed to create!"); break;
	default: BX_LOGE("Failed to create directory: %s", strerror(errno)); break;
	}
	return false;
#else
	BX_ASSERT(false, "Create directory not supported!");
	return false;
#endif
}

u64 File::LastWrite(const String& filename)
{
#if (defined(BX_PLATFORM_PC) || defined(BX_PLATFORM_LINUX)) \
&& (defined(BX_BUILD_DEBUG) || defined(BX_BUILD_PROFILE))
	const auto filepath = GetPath(filename);
	struct stat info;
	if (stat(filename.c_str(), &info) == 0)
	{
		return info.st_mtime;
	}
#endif

	return 0;
}

bool File::ListFiles(const String& root, List<FileHandle>& files)
{
#if defined(BX_PLATFORM_PC)

	String rootPath = GetPath(root);

	if (rootPath.size() > (MAX_PATH - 3))
		return false;

	TCHAR szDir[MAX_PATH];
	StringCchCopy(szDir, MAX_PATH, rootPath.c_str());
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(szDir, &ffd);
	//HANDLE hFind = FindFirstFileEx(szDir, FindExInfoBasic, &ffd, FindExSearchNameMatch, nullptr, 0);
	
	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		FileHandle fh;
		fh.filepath = root + "/" + ffd.cFileName;
		fh.filename = ffd.cFileName;

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (strcmp(ffd.cFileName, ".") == 0) continue;
			if (strcmp(ffd.cFileName, "..") == 0) continue;

			fh.isDirectory = true;
			files.emplace_back(fh);
		}
		else
		{
			fh.isDirectory = false;
			files.emplace_back(fh);
		}

	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
	return true;

#elif defined(BX_PLATFORM_LINUX)

	String rootPath = GetPath(root);

	DIR* dir = opendir(rootPath.c_str());
	if (dir == NULL)
		return false;

	struct dirent* ent;
	while ((ent = readdir(dir)) != NULL)
	{
		FileHandle fh;
		fh.filepath = root + "/" + ent->d_name;
		fh.filename = ent->d_name;

		if (ent->d_type == DT_DIR)
		{
			if (strcmp(ent->d_name, ".") == 0) continue;
			if (strcmp(ent->d_name, "..") == 0) continue;

			fh.isDirectory = true;
			files.emplace_back(fh);
		}
		else
		{
			fh.isDirectory = false;
			files.emplace_back(fh);
		}
	}

	closedir(dir);
	return true;

#else

	BX_ASSERT(false, "Find file not supported!");
	return false;

#endif
}

bool File::Find(const String& root, const String& filename, String& filepath)
{
	List<FileHandle> files;
	if (!ListFiles(root, files))
		return false;

	for (const auto& file : files)
	{
		if (file.isDirectory)
		{
			if (File::Find(file.filepath, filename, filepath))
			{
				return true;
			}
		}
		else
		{
			if (file.filename == filename)
			{
				filepath = file.filepath;
				return true;
			}
		}
	}

	return false;
}

void File::FindEach(const String& root, const String& ext, const FindEachCallback& callback)
{
	List<FileHandle> files;
	if (!ListFiles(root, files))
		return;

	for (const auto& file : files)
	{
		if (file.isDirectory)
		{
			FindEach(file.filepath, ext, callback);
			continue;
		}
		
		std::size_t split = file.filename.find_last_of(".");
		const auto& fileName = file.filename.substr(0, split);
		const auto& fileExt = file.filename.substr(split);
		if (fileExt == ext)
		{
			callback(file.filepath, fileName);
		}
	}
}