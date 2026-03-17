#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include <windows.h>

class FileManager
{
public:
    static FileManager &instance();

    void ensureDirectory(const std::filesystem::path& path);
    void ensureFileExists(const std::string &path, bool isRegular);
    void ensureReadable(const std::string &path);
    void ensureWritable(const std::string &path);
    void write(const std::string &path, const std::string &text);
    bool exists(const std::string &path) const;
    void createFile(const std::filesystem::path & path);
    void ensureParentDirectory(const std::filesystem::path& path);
    std::filesystem::path getExeDir();

private:
    FileManager() = default;
    ~FileManager() = default;

    FileManager(const FileManager &) = delete;
    FileManager &operator=(const FileManager &) = delete;
};