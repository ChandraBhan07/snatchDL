#pragma once
#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>

#include "fileManager.h"

class Logger
{

private:
    const std::filesystem::path logpath;
    void initDir() const;
    void log(const std::string &source, const std::string &id, const std::string &status);

public:
    Logger(const std::filesystem::path &path) : logpath(std::move(path))
    {
        initDir();
    };
    void failure(const std::string &source, const std::string &id);
    void success(const std::string &source, const std::string &id);
};