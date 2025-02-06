#pragma once

#include <engine/api.hpp>
#include <engine/module.hpp>
#include <engine/string.hpp>
#include <engine/list.hpp>
#include <engine/hash_map.hpp>

#include <fstream>

using FilePath = CString<256>;
using FileName = CString<64>;

using InputFileStream = std::ifstream;
using OutputFileStream = std::ofstream;

using FindEachCallback = std::function<void(const FilePath& path, const FileName& name)>;

struct BX_API FileHandle
{
    FilePath filepath{};
    FileName filename{};
    bool isDirectory{ false };
};

struct BX_API MountPoint
{
    FileName name{};
    FilePath path{};
};

class BX_API File
{
    BX_MODULE(File)

public:
    bool Initialize();
    void Shutdown();

    void AddMountPoint(StringView mountName, StringView path);

    bool Exists(StringView path);
    u64 LastWrite(StringView filename);

    FilePath GetPath(StringView filename);

    StringView GetExt(StringView filename);
    StringView RemoveExt(StringView filename);

    StringView GetFilename(StringView file);
    List<StringView> SplitPath(StringView path, char delimiter = '/');

    bool Move(StringView oldPath, StringView newPath);
    bool Delete(StringView path);

    bool CreateDirectory(StringView path);

    bool ListFiles(StringView root, List<FileHandle>& files);
    bool Find(StringView root, StringView filename, FilePath& filepath);
    void FindEach(StringView root, StringView ext, const FindEachCallback& callback);
    
    String ReadText(StringView filename);
    bool WriteText(StringView filename, StringView text);

    List<char> ReadBinary(StringView filename);

private:
    HashMap<FileName, List<MountPoint>> m_mountPoints{};
};