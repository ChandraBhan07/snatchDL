#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <random>
#include <fstream>
#include <filesystem>
#include <unordered_set>

#include "cookie.h"
#include "fileManager.h"
#include "source.h"

class CookieManager
{
public:
    // Acquire a cookie if one is safely usable.
    // Returning nullptr is a valid and expected outcome.
    Cookie *acquire(const std::string &source);

    // Reporting outcomes (must be called after use)
    void report_success(Cookie &cookie);
    void report_soft_failure(Cookie &cookie);
    void report_hard_failure(Cookie &cookie);

    int getPoolSize() const
    {
        int size = 0;
        for(auto const itr: pool) size += itr.second.size();
        return size;
    }
    void scanCookies(const std::filesystem::path &path);

private:
    bool isValidNetscapeFile(const std::filesystem::path &path);
    void add_cookie(const std::string &cname, const std::filesystem::path &cpath, const std::string &source);
    std::unordered_map<std::string, std::vector<Cookie>> pool;

    void rest(Cookie &cookie, std::chrono::seconds min,
              std::chrono::seconds max);
    std::string detectPlatform(const std::filesystem::path &path, std::unordered_set<std::string> & sources);
};
