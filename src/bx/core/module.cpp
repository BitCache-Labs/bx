#include <bx/core/module.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

std::vector<std::string> Module::list_library_names(const std::string& directory)
{
    std::vector<std::string> libraryNames;

#ifdef _WIN32
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((directory + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("Failed to open directory: " + directory);
    }

    do
    {
        std::string filename = findFileData.cFileName;
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (filename.find(".dll") != std::string::npos)
            {
                libraryNames.push_back(directory + "\\" + filename.substr(0, filename.find(".dll")));
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
#else
    DIR* dir;
    struct dirent* entry;
    struct stat info;

    if ((dir = opendir(directory.c_str())) == nullptr)
    {
        throw std::runtime_error("Failed to open directory: " + directory);
    }

    while ((entry = readdir(dir)) != nullptr)
    {
        std::string filename = entry->d_name;
        std::string fullPath = directory + "/" + filename;

        if (stat(fullPath.c_str(), &info) != 0 || S_ISDIR(info.st_mode))
        {
            continue;
        }

        if (filename.find(".so") != std::string::npos)
        {
            libraryNames.push_back(directory + "/" + filename.substr(0, filename.find(".so")));
        }
    }

    closedir(dir);
#endif

    return libraryNames;
}