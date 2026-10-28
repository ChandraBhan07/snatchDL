#pragma once

#include <filesystem>
#include <vector>

class InputFile
{
    std::vector<std::string> content;
    const std::filesystem::path filePath;

public:
    InputFile(const std::filesystem::path &path) : filePath(path) {};
    void append(const std::string &text);
    void read();

    const std::vector<std::string> &getContent() const
    {
        return content;
    }
};