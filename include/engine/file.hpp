#pragma once

#include <engine/api.hpp>
#include <engine/module.hpp>
#include <engine/string.hpp>
#include <engine/list.hpp>
#include <engine/log.hpp>

#include <fstream>

LOG_CHANNEL(File)

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
    BX_MODULE_INTERFACE(File)

public:
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    virtual void AddMountPoint(StringView mountName, StringView path) = 0;

    virtual bool Exists(StringView path) = 0;
    virtual u64 LastWrite(StringView filename) = 0;

    virtual FilePath GetPath(StringView filename) = 0;

    virtual StringView GetExt(StringView filename) = 0;
    virtual StringView RemoveExt(StringView filename) = 0;

    virtual StringView GetFilename(StringView file) = 0;
    virtual List<StringView> SplitPath(StringView path, char delimiter = '/') = 0;

    virtual bool Move(StringView oldPath, StringView newPath) = 0;
    virtual bool Delete(StringView path) = 0;

    virtual bool CreateDirectory(StringView path) = 0;

    virtual bool ListFiles(StringView root, List<FileHandle>& files) = 0;
    virtual bool Find(StringView root, StringView filename, FilePath& filepath) = 0;
    virtual void FindEach(StringView root, StringView ext, const FindEachCallback& callback) = 0;
    
    virtual String ReadText(StringView filename) = 0;
    virtual bool WriteText(StringView filename, StringView text) = 0;

    virtual List<char> ReadBinary(StringView filename) = 0;

    virtual OutputFileStream OutputStream(const StringView& filename) = 0;
    virtual InputFileStream InputStream(const StringView& filename) = 0;
};