#include "source.h"

Source &Source::instance()
{
    static Source inst;
    return inst;
}

Source::Source()
{
    host_to_source["youtube.com"] = "yt";
    host_to_source["youtu.be"] = "yt";
    cnn_forms["yt"] = "https://www.youtube.com/v";

    host_to_source["instagram.com"] = "ig";
    cnn_forms["ig"] = "https://www.instagram.com/p";

    host_to_source["twitter.com"] = "tw";
    host_to_source["x.com"] = "tw";
    cnn_forms["tw"] = "https://www.twitter.com/status";

    host_to_source["tiktok.com"] = "tt";
    cnn_forms["tt"] = "https://www.tiktok.com/video";

    host_to_source["vimeo.com"] = "vm";
    cnn_forms["vm"] = "https://www.vimeo.com/video";

    host_to_source["facebook.com"] = "fb";
    cnn_forms["fb"] = "https://www.facebook.com/video";

    // Extend here:
    // host_to_source[""] = "";
}

std::string Source::extract_host(const std::string &url) const
{
    auto start = url.find("://");
    start = (start == std::string::npos) ? 0 : start + 3;

    auto end = url.find('/', start);
    auto host = url.substr(start, end - start);

    // strip port if present
    auto colon = host.find(':');
    if (colon != std::string::npos)
        host = host.substr(0, colon);

    return host;
}

std::string Source::normalize_host(std::string host) const
{
    std::transform(host.begin(), host.end(), host.begin(), ::tolower);

    static const std::string prefixes[] = {
        "www.", "m.", "mobile."};

    for (const auto &p : prefixes)
    {
        if (host.rfind(p, 0) == 0)
        {
            host.erase(0, p.size());
            break;
        }
    }

    return host;
}

// normalize domain is for cookie files -
// .instagram results in instagram.com
// standard netscape format
std::string Source::normalizeDomain(const std::string &domain)
{
    std::string d = domain;

    if (d[0] == '.')
        d = d.substr(1);

    std::transform(d.begin(), d.end(), d.begin(), ::tolower);

    // simple root extraction
    size_t pos = d.find_last_of('.');
    if (pos == std::string::npos)
        return d;

    size_t pos2 = d.find_last_of('.', pos - 1);
    if (pos2 == std::string::npos)
        return d;

    return d.substr(pos2 + 1);
}

std::string Source::extract_path(const std::string &url) const
{
    auto start = url.find("://");
    start = (start == std::string::npos) ? 0 : start + 3;

    start = url.find('/', start);
    if (start == std::string::npos)
        return "";

    auto end = url.find('?', start);
    return url.substr(start, end - start);
}

std::string Source::extract_query(const std::string &url) const
{
    auto q = url.find('?');
    if (q == std::string::npos)
        return "";

    auto end = url.find('#', q);
    return url.substr(q + 1, end - q - 1);
}

std::string Source::get_query_param(const std::string &query,
                                    const std::string &key) const
{
    size_t pos = 0;
    while (pos < query.size())
    {
        auto eq = query.find('=', pos);
        if (eq == std::string::npos)
            break;

        auto name = query.substr(pos, eq - pos);
        auto amp = query.find('&', eq);

        auto value = query.substr(eq + 1,
                                  (amp == std::string::npos ? query.size() : amp) - eq - 1);

        if (name == key)
            return value;

        pos = (amp == std::string::npos) ? query.size() : amp + 1;
    }
    return "";
}

std::string Source::getSource(const std::string &host)
{
    std::string source = "others";
    auto it = host_to_source.find(host);
    if (it != host_to_source.end())
        source = it->second;
    return source;
}

std::pair<std::string, std::string> Source::resolveUrl(const std::string &url)
{
    std::string id;
    auto host = normalize_host(extract_host(url));
    std::string source = getSource(host);

    if (source == "yt")
    {
        auto query = extract_query(url);
        id = get_query_param(query, "v");

        if (id.empty())
        {
            auto path = extract_path(url);
            if (!path.empty())
                id = path.substr(1);
        }
    }

    else if (source == "ig")
    {
        auto path = extract_path(url);

        static const std::string_view ig_prefixes[] = {
            "/p/",
            "/reel/",
            "/tv/"};

        for (std::string_view prefix : ig_prefixes)
        {
            auto pos = path.find(prefix);
            if (pos != std::string::npos)
            {
                auto start = pos + prefix.size();
                auto end = path.find('/', start);
                id = path.substr(start, end - start);
                break;
            }
        }
    }

    else if (source == "tw")
    {
        auto path = extract_path(url);
        auto pos = path.find("/status/");
        if (pos != std::string::npos)
        {
            auto start = pos + 8;
            auto end = path.find('/', start);
            id = path.substr(start, end - start);
        }
    }

    return {source, id};
}


std::string Source::getCanonicalForm(const std::string &source)
{
    std::string cnn = "";
    auto it = cnn_forms.find(source);
    if (it != cnn_forms.end())
        cnn = it->second;
    return cnn;
}