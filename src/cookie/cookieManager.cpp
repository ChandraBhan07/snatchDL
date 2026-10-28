#include "cookieManager.h"
#include "cookie.h"
#include "../config/config.h"
#include "../service/serviceRegistry.h"
#include "../runtime/spread.h"
#include <fstream>

void CookieManager::scanCookies()
{
    for (const auto &entry : std::filesystem::directory_iterator(cookiesDir))
    {
        if (!entry.is_regular_file())
            continue;

        // std::cout << "entry " << entry << "\n";
        // std::cout << "entry path " << entry.path() << "\n";
        // std::cout << "entry stem " << entry.path().stem() << "\n";

        auto cookie = std::make_unique<Cookie>(entry.path());

        std::string host;
        if (isValidNetscapeFile(entry.path(), host))
        {
            host.erase(0, 1); // removes the first dot

            Service *service = registry.findByHost(host);

            // later deal with other cookies
            if (service)
                service->cookies.insert(std::move(cookie));
        }
        else
        {
            // implement later for other services cookies
        }
    }
}

// return something like .instagram.com
bool CookieManager::isValidNetscapeFile(const std::filesystem::path &path, std::string &host)
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
        {
            fieldCount++;
            if (host.empty() && !field.empty() && field[0] == '.')
            {
                host = field;
            }
        }

        if (fieldCount == 7)
            validLines++;
    }

    return validLines > 0;
}

Cookie *CookieManager::acquire(const std::string &serviceKey)
{
    Cookie *candidate = nullptr;

    const Service *service = registry.findByKey(serviceKey);

    if (service)
        for (auto &c : service->cookies)
        {
            if (!c.get()->ready())
                continue;

            if (!candidate)
                candidate = c.get();
            else if (c.get()->lastUsed < candidate->lastUsed)
                candidate = c.get();
        }

    return candidate;
}

void CookieManager::rest(Cookie *cookie, std::chrono::seconds min, std::chrono::seconds max)
{
    cookie->state = Cookie::State::RESTING;
    cookie->lastUsed = std::chrono::steady_clock::now();

    static thread_local std::mt19937 rng(std::random_device{}());
    int wait = Spread::random(min.count(), max.count(), rng);

    cookie->nextAllowed = cookie->lastUsed + std::chrono::seconds(wait);
}

void CookieManager::reportSuccess(Cookie *cookie)
{
    cookie->trust = std::min(100, cookie->trust + 2);
    rest(cookie, std::chrono::seconds(48),
         std::chrono::seconds(117));
}

void CookieManager::reportSoftFailure(Cookie *cookie)
{
    cookie->trust -= 10;

    if (cookie->trust <= 30)
    {
        cookie->state = Cookie::State::RETIRED;
        return;
    }

    rest(cookie, std::chrono::seconds(120),
         std::chrono::seconds(360));
}

void CookieManager::reportHardFailure(Cookie *cookie)
{
    cookie->trust = 0;
    cookie->state = Cookie::State::RETIRED;
}

// temporary reset cookie's states until cookie manager is fixed
void CookieManager::resetCookieStates()
{
    const Service *service = registry.findByKey("ig");

    for (auto &c : service->cookies)
    {
        c->state = Cookie::State::ACTIVE;
        c->nextAllowed = std::chrono::steady_clock::now();
    }
}