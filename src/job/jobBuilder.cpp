#include "jobBuilder.h"
#include "job.h"
#include "jobHistory.h"
#include "../service/serviceRegistry.h"
#include "../url/url.h"

#include <iostream>

// later add many filters as needed
std::optional<Job> JobBuilder::build(std::string text)
{
    if (!Url::isUrl(text))
        return std::nullopt;

    Url::Parts parts = Url::parse(text);

    if (!parts.scheme.empty() && parts.scheme != "https")
        return std::nullopt;

    const Service *service = registry.findByHost(parts.host);

    std::string key;
    std::string id;
    std::string url;

    if (service)
    {
        key = service->key;
        id = service->extractId(parts);
        url = service->makeUrl(id);
    }
    else
    {
        key = "others";
        id = Url::discardFragment(parts);
        url = id;
    }

    if (id.empty())
        return std::nullopt;

    auto itr = seen.find(key);
    if (itr != seen.end())
    {
        if (itr->second.count(id))
            return std::nullopt;

        itr->second.insert(id);
    }
    else
        seen[key] = {};

    if (history.contains(key, id))
        return std::nullopt;

    Job job(id, key, url);

    return job;
}