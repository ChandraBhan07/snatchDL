#include "logger.h"

void Logger::initDir() const
{
    FileManager::instance().ensureDirectory(logpath);
}

void Logger::success(const std::string &source, const std::string &id)
{
    log(source, id, "succeed");
}

void Logger::failure(const std::string &source, const std::string &id)
{
    log(source, id, "failed");
}

void Logger::log(const std::string &source, const std::string &id, const std::string &status)
{
    std::string filename = (logpath / status / source).string() + ".logs";
    FileManager::instance().ensureWritable(filename);
    FileManager::instance().write(filename, id);
}