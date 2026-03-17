#include "cookieManager.h"
#include <algorithm>
#include <iostream>

auto &fm = FileManager::instance();


void CookieManager::add_cookie(const std::string &cname, const std::filesystem::path &cpath, const std::string &source)
{
    pool[source].emplace_back(Cookie(cname, cpath, source));
}

Cookie *CookieManager::acquire(const std::string &source)
{
    auto it = pool.find(source);
    if (it == pool.end())
        return nullptr;

    Cookie *candidate = nullptr;

    for (auto &c : it->second)
    {
        if (!c.ready())
            continue;

        if (!candidate)
            candidate = &c;
        else if (c.last_used < candidate->last_used)
            candidate = &c;
    }

    return candidate;
}

void CookieManager::rest(Cookie &cookie,
                         std::chrono::seconds min,
                         std::chrono::seconds max)
{
    cookie.state = Cookie::State::RESTING;
    cookie.last_used = std::chrono::system_clock::now();

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(min.count(), max.count());
    int wait_minutes = dist(rng);

    cookie.next_allowed = cookie.last_used + std::chrono::seconds(wait_minutes);
}

void CookieManager::report_success(Cookie &cookie)
{
    cookie.trust = std::min(100, cookie.trust + 2);
    // rest(cookie, std::chrono::minutes(30),
    //              std::chrono::minutes(90));
    rest(cookie, std::chrono::seconds(48),
         std::chrono::seconds(117));
}

void CookieManager::report_soft_failure(Cookie &cookie)
{
    cookie.trust -= 10;

    if (cookie.trust <= 40)
    {
        cookie.state = Cookie::State::RETIRED;
        return;
    }

    rest(cookie, std::chrono::seconds(120),
         std::chrono::seconds(360));
}

void CookieManager::report_hard_failure(Cookie &cookie)
{
    cookie.trust = 0;
    cookie.state = Cookie::State::RETIRED;
}

bool CookieManager::isValidNetscapeFile(const std::filesystem::path &path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string firstLine;
    std::getline(file, firstLine);

    if (firstLine.find("Netscape HTTP Cookie File") == std::string::npos)
        return false;

    std::string line;
    int validLines = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string field;
        int fieldCount = 0;

        while (std::getline(ss, field, '\t'))
            fieldCount++;

        if (fieldCount == 7)
            validLines++;
    }

    return validLines > 0;
}

void CookieManager::scanCookies(const std::filesystem::path &path)
{
    fm.ensureDirectory(path);
    
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_regular_file())
        continue;
        
        std::unordered_set<std::string> sources;
        if (isValidNetscapeFile(entry))
        {
            detectPlatform(entry, sources);
        }

        for(const std::string source:sources){
            add_cookie(entry.path().stem().string(), entry, source);
        }

    }
}

std::string CookieManager::detectPlatform(const std::filesystem::path &path, std::unordered_set<std::string> &sources)
{
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string domain;
        std::getline(ss, domain, '\t');

        std::string root = Source::instance().normalizeDomain(domain);
        std::string source = Source::instance().getSource(root);
        sources.insert(source);
    }

    return "";
}