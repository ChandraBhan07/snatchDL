#include "FileManager.h"

namespace fs = std::filesystem;

FileManager &FileManager::instance()
{
    static FileManager instance; // Meyers Singleton (thread-safe)
    return instance;
}

bool FileManager::exists(const std::string &path) const
{
    return fs::exists(path);
}

void FileManager::ensureDirectory(const std::filesystem::path& path)
{
    try
    {
        if (!fs::exists(path))
        {
            if (!fs::create_directories(path))
            {
                throw std::runtime_error(
                    "Failed to create directory '" + path.string() + "'");
            }
        }
        else if (!fs::is_directory(path))
        {
            throw std::runtime_error(
                "'" + path.string() + "' exists but is not a directory");
        }
    }
    catch (const fs::filesystem_error &e)
    {
        throw std::runtime_error(e.what());
    }
}

void FileManager::ensureFileExists(const std::string &path, bool isRegular = true)
{
    if (!fs::exists(path))
    {
        throw std::runtime_error(
            "File '" + path + "' does not exist");
    }

    if (isRegular && !fs::is_regular_file(path))
    {
        throw std::runtime_error(
            "'" + path + "' exists but is not a regular file");
    }
}

void FileManager::ensureReadable(const std::string &path)
{
    ensureFileExists(path);

    std::ifstream file(path);
    if (!file.good())
    {
        throw std::runtime_error(
            "File '" + path + "' is not readable");
    }
}

void FileManager::ensureWritable(const std::string &path)
{
    std::ofstream file(path, std::ios::app);
    if (!file.good())
    {
        throw std::runtime_error(
            "File '" + path + "' is not writable");
    }
}

void FileManager::write(const std::string &path, const std::string &text)
{
    ensureWritable(path);
    std::ofstream file(path, std::ios::app);
    file << text + "\n";
    file.close();
}

void FileManager::createFile(const fs::path &path)
{
   
    ensureParentDirectory(path);

    // If file already exists, ensure it is a regular file
    if (fs::exists(path))
    {
        if (!fs::is_regular_file(path))
        {
            throw std::runtime_error(
                "'" + path.string() + "' exists but is not a regular file");
        }
        return; // nothing to do
    }

    // Create empty file
    std::ofstream file(path);
    if (!file.good())
    {
        throw std::runtime_error(
            "Failed to create file '" + path.string() + "'");
    }
}

void FileManager::ensureParentDirectory(const std::filesystem::path& path)
{
    // Parent directory must exist
    if (!path.parent_path().empty() && !fs::exists(path.parent_path()))
    {
        ensureDirectory(path.parent_path().string());
    }
}

std::filesystem::path FileManager::getExeDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}