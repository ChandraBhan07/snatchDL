#pragma once

#include <filesystem>

struct Config;
struct Service;
class ServiceRegistry;
class Cookie;

class CookieManager
{
    ServiceRegistry &registry;
    const std::filesystem::path &cookiesDir;
    bool isValidNetscapeFile(const std::filesystem::path &path, std::string &host);

public:
    CookieManager(ServiceRegistry &reg, const std::filesystem::path &path)
        : registry(reg), cookiesDir(path) {}

    void scanCookies();
    Cookie *acquire(const std::string& serviceKey);
    void rest(Cookie *cookie, std::chrono::seconds min, std::chrono::seconds max);
    void reportSuccess(Cookie *cookie);
    void reportSoftFailure(Cookie *cookie);
    void reportHardFailure(Cookie *cookie);

    // delete later
    void resetCookieStates();
};