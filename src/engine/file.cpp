#include <engine/file.hpp>
#include <engine/log.hpp>
#include <engine/guard.hpp>

// TODO: Hacked solution for now
#define BX_PLATFORM_PC

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

#include <sstream>

BX_MODULE_DEFINE(File)

bool File::Initialize()
{
#if defined(BX_PLATFORM_PC) || defined(BX_PLATFORM_LINUX)
    AddWildcard("[assets]", GAME_PATH"/assets");
    AddWildcard("[settings]", GAME_PATH"/settings");

    // All platforms
    if (!Exists(GetPath("[settings]/.ini")))
    {
        BX_LOGW(File, "Not game .ini file, creating default.");
        WriteText(GetPath("[settings]/.ini"), "GameName");
    }

    auto gameStr = ReadText(GetPath("[settings]/.ini"));
    if (gameStr.empty())
    {
        gameStr = "GameName";
        BX_LOGW(File, "Game .ini does not have game name, setting to default.");
        WriteText(GetPath("[settings]/.ini"), gameStr);
    }
#endif

#if defined(BX_PLATFORM_PC)
    char* pValue;
    size_t len;
    _dupenv_s(&pValue, &len, "APPDATA");

    if (pValue != nullptr)
    {
        CString<512> save_path{};
        save_path.format("{}/{}/", pValue, gameStr);

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

//#ifdef EDITOR_BUILD
//    AddWildcard("[editor]", GAME_PATH"/editor");
//#endif

    return true;
}

void File::Shutdown()
{
}

void File::AddWildcard(StringView wildcard, StringView value)
{
    if (!Exists(value))
    {
        if (!CreateDirectory(value))
        {
            BX_LOGE(File, "Failed to create directory!");
            BX_FAIL("Create directory failed!");
        }
    }
    m_wildcards[wildcard] = value;
}

bool File::Exists(StringView path)
{
    const auto filepath = GetPath(path);
    struct stat info;
    return (stat(filepath.c_str(), &info) == 0);
}

u64 File::LastWrite(StringView filename)
{
#if (defined(BX_PLATFORM_PC) || defined(BX_PLATFORM_LINUX))
    const auto filepath = GetPath(filename);
    struct stat info;
    if (stat(filepath.c_str(), &info) == 0)
    {
        return info.st_mtime;
    }
#endif

    return 0;
}

StringView File::GetFilename(StringView file)
{
    SizeType lastSlash = file.find_last_of("/\\");
    if (lastSlash == std::string::npos)
        return file;

    return file.substr(lastSlash + 1);
}

CString<512> File::GetPath(StringView filename)
{
    CString<512> filepath{ filename };

    for (const auto& p : m_wildcards)
    {
        auto pos = filepath.find(p.first);
        if (pos != String::npos)
            filepath.replace(pos, p.first.length(), p.second);
    }

    return filepath;
}

StringView File::GetExt(StringView filename)
{
    return filename.substr(filename.find_last_of(".") + 1);
}

StringView File::RemoveExt(StringView filename)
{
    return filename.substr(0, filename.find_last_of("."));
}

List<StringView> File::SplitPath(StringView path, char delimiter)
{
    List<StringView> parts;
    SizeType start = 0;
    SizeType end;

    while ((end = path.find(delimiter, start)) != String::npos)
    {
        parts.emplace_back(path.substr(start, end - start));
        start = end + 1;
    }

    if (start < path.size())
        parts.emplace_back(path.substr(start));

    return parts;
}

bool File::Move(StringView oldPath, StringView newPath)
{
    String oldFullPath = File::GetPath(oldPath);
    String newFullPath = File::GetPath(newPath);

#if defined(BX_PLATFORM_PC)
    LPCSTR oldFileName = oldFullPath.c_str();
    LPCSTR newFileName = newFullPath.c_str();

    // Attempt to move the file with additional options
    if (MoveFileEx(oldFileName, newFileName, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
    {
        BX_LOGD(File, "File moved old: ({}) -> new: ({})", oldPath, newPath);
        return true;
    }

    BX_LOGW(File, "Failed to move file: {}", GetLastError());
    return false;

#elif defined(BX_PLATFORM_LINUX)
    BX_FAIL("Delete file not supported!");
    return false;

#else
    BX_FAIL("Delete file not supported!");
    return false;

#endif
}

bool File::Delete(StringView filename)
{
#if defined(BX_PLATFORM_PC)
    const String fullpath = GetPath(filename);

    DWORD attributes = GetFileAttributes(fullpath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        BX_LOGD(File, "Invalid path or unable to get attributes. Error code: {}", GetLastError());
        return false;
    }

    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (RemoveDirectory(fullpath.c_str()))
        {
            BX_LOGD(File, "Folder deleted: {}", filename);
        }
    }
    else
    {
        if (DeleteFile(fullpath.c_str()))
        {
            BX_LOGD(File, "File deleted: {}", filename);
            return true;
        }
    }

    BX_LOGW(File, "Failed to delete file: {}", filename);
    return false;

#elif defined(BX_PLATFORM_LINUX)
    BX_FAIL("Delete file not supported!");
    return false;

#else
    BX_FAIL("Delete file not supported!");
    return false;

#endif
}

bool File::CreateDirectory(StringView path)
{
#if defined(BX_PLATFORM_PC)
    BOOL ret = WinCreateDirectory(path.data(), NULL);
    switch (GetLastError())
    {
    case ERROR_ALREADY_EXISTS: BX_LOGE(File, "Directory already exists, failed to create!"); break;
    case ERROR_PATH_NOT_FOUND: BX_LOGE(File, "Directory path not found, failed to create!"); break;
    }
    return ret;

#elif defined(BX_PLATFORM_LINUX)
    int ret = mkdir(path.c_str(), 0755);
    if (ret == 0)
        return true;

    switch (errno)
    {
    case EEXIST: BX_LOGE(File, "Directory already exists, failed to create!"); break;
    case ENOENT: BX_LOGE(File, "Directory path not found, failed to create!"); break;
    default: BX_LOGE(File, "Failed to create directory: %s", strerror(errno)); break;
    }
    return false;
#else
    BX_FAIL("Create directory not supported!");
    return false;
#endif
}

bool File::ListFiles(StringView root, List<FileHandle>& files)
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
        fh.filepath.format("{}/{}", root, ffd.cFileName);
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

bool File::Find(StringView root, StringView filename, CString<512>& filepath)
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

void File::FindEach(StringView root, StringView ext, const FindEachCallback& callback)
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

        SizeType split = file.filename.find_last_of(".");
        const auto& fileName = file.filename.substr(0, split);
        const auto& fileExt = file.filename.substr(split);
        if (fileExt == ext)
        {
            callback(file.filepath, fileName);
        }
    }
}

String File::ReadText(StringView filename)
{
    // Open the file in input mode.
    InputFileStream ifs(filename.data(), std::ios::in);
    if (!ifs.is_open())
    {
        BX_LOGE(File, "Failed to open file: {}", filename);
        return String();
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    if (!ifs)
    {
        BX_LOGE(File, "Failed to read from file: {}", filename);
        return String();
    }

    return String(buffer.str());
}

bool File::WriteText(StringView filename, StringView text)
{
    // Open the file in write mode (creating it if it doesn't exist)
    OutputFileStream ofs(filename.data(), std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
    {
        BX_LOGE(File, "Failed to open file: {}", filename);
        return false;
    }

    ofs.write(text.data(), text.size());
    if (!ofs)
    {
        BX_LOGE(File, "Failed to write to file: {}", filename);
        return false;
    }

    ofs.close();
    return true;
}

List<char> File::ReadBinary(StringView filename)
{
    const auto path = GetPath(filename);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        BX_LOGE(File, "File {} with full path {} was not found!", filename, path);
        return List<char>();
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    List<char> buffer(size);
    if (file.read(buffer.data(), size))
        return buffer;

    BX_FAIL("");
    return List<char>();
}