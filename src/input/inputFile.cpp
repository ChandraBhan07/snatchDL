#include "inputFile.h"

#include <fstream>
#include <algorithm>

void InputFile::read()
{
    std::ifstream file(filePath);
    if (!file)
        throw std::runtime_error("Cannot open file: " + filePath.string());

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        line.erase(
            line.begin(),
            std::find_if(line.begin(), line.end(),
                         [](unsigned char ch)
                         {
                             return !std::isspace(ch);
                         }));

        line.erase(
            std::find_if(line.rbegin(), line.rend(),
                         [](unsigned char ch)
                         {
                             return !std::isspace(ch);
                         })
                .base(),
            line.end());

        if (line.empty())
            continue;

        content.push_back(line);
    }
}

void InputFile::append(const std::string &text)
{
    std::ofstream file(filePath, std::ios::app);
    if (!file)
        throw std::runtime_error("Cannot write to file: " + filePath.string());

    file << text + '\n';
    file.close();
}