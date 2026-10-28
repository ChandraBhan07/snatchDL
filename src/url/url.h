#pragma once
#include <string>

namespace Url
{
    struct Parts
    {
        std::string scheme;
        std::string host;
        std::string path;
        std::string query;
        std::string fragment;
    };

    bool isUrl(const std::string &input);
    Parts parse(const std::string &url);
    std::string discardFragment(const Parts &parts);
}