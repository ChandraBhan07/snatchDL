#pragma once

#include "../url/url.h"
#include "../cookie/cookie.h"

#include <unordered_set>
#include <functional>
#include <filesystem>
#include <memory>



// notes - dont make aliases global and right now I am going with direct types and not alias
// extract id takes urlparts and extract Id, exaction is different for every platform
// makeUrl = canonical url + id

struct Service
{

    std::string name;
    std::string key;
    std::unordered_set<std::string> hosts;
    std::unordered_set<std::unique_ptr<Cookie>> cookies;

    //  using IdExtractor = std::function<std::string(const ParsedUrl&)>;
    // using UrlBuilder  = std::function<std::string(const std::string&)>;
    // IdExtractor extractId;
    // UrlBuilder makeUrl;
    // OR

    std::function<std::string(const Url::Parts &)> extractId;
    std::function<std::string(const std::string &)> makeUrl;
};