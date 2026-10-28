#include "serviceRegistry.h"

// two stages, first all service objects and then the hosttoservice map
ServiceRegistry::ServiceRegistry()
{
    services.emplace("yt", Service{"YouTube",
                                   "yt",
                                   {"youtube.com", "youtu.be", "www.youtube.com", "m.youtube.com"},
                                   {},

                                   [](const Url::Parts &parts) -> std::string
                                   {
                                       if (parts.host == "youtu.be")
                                       {
                                           if (parts.path.size() > 1)
                                               return parts.path.substr(1);

                                           return {};
                                       }

                                       // youtube.com/watch?v=...
                                       const std::string key = "v=";
                                       size_t pos = parts.query.find(key);

                                       if (pos == std::string::npos)
                                           return {};

                                       pos += key.size();

                                       size_t end = parts.query.find('&', pos);

                                       return parts.query.substr(pos, end == std::string::npos
                                                                          ? std::string::npos
                                                                          : end - pos);
                                   },

                                   [](const std::string &id)
                                   {
                                       return "https://www.youtube.com/v/" + id;
                                   }});

    services.emplace("ig", Service{"Instagram",
                                   "ig",
                                   {"instagram.com", "www.instagram.com"},
                                   {},

                                   [](const Url::Parts &parts) -> std::string
                                   {
                                       std::string path = parts.path;

                                       if (!path.empty() && path.front() == '/')
                                           path.erase(0, 1);

                                       size_t first = path.find('/');

                                       if (first == std::string::npos)
                                           return {};

                                       std::string type = path.substr(0, first);

                                       if (type != "p" && type != "reel" && type != "reels")
                                           return {};

                                       size_t start = first + 1;

                                       size_t end = path.find('/', start);

                                       return path.substr(start, end == std::string::npos
                                                                     ? std::string::npos
                                                                     : end - start);
                                   },

                                   [](const std::string &id)
                                   {
                                       return "https://www.instagram.com/p/" + id;
                                   }

                           });

    services.emplace("tw", Service{"Twitter",
                                   "tw",
                                   {"twitter.com", "www.twitter.com", "x.com", "www.x.com"},
                                   {},

                                   [](const Url::Parts &parts) -> std::string
                                   {
                                       const std::string marker = "/status/";

                                       size_t pos = parts.path.find(marker);

                                       if (pos == std::string::npos)
                                           return {};

                                       pos += marker.size();

                                       size_t end = parts.path.find('/', pos);

                                       return parts.path.substr(pos, end == std::string::npos
                                                                         ? std::string::npos
                                                                         : end - pos);
                                   },

                                   [](const std::string &id)
                                   {
                                       return "https://www.twitter.com/status/" + id;
                                   }});

    for (auto &[key, service] : services)
    {
        for (const auto &host : service.hosts)
            hostToService.emplace(host, &service);
    }
}

const Service *ServiceRegistry::findByKey(const std::string &key) const
{
    auto it = services.find(key);

    if (it == services.end())
        return nullptr;

    return &it->second;
}

 Service *ServiceRegistry::findByHost(const std::string &host) const
{
    auto it = hostToService.find(host);

    if (it == hostToService.end())
        return nullptr;

    return it->second;
}
