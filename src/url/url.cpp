#include "url.h"

#include <regex>

namespace Url
{
    bool isUrl(const std::string &input)
    {
        if (input.empty())
            return false;

        // Clipboard value must be a single URL-like string.
        if (input.find_first_of(" \t\r\n") != std::string::npos)
            return false;

        // Explicit scheme.
        static const std::regex withScheme(
            R"(^(https?|ftp|rtmp|rtmps|rtsp|mms|file)://.+$)",
            std::regex::icase);

        if (std::regex_match(input, withScheme))
            return true;

        // domain, localhost, IPv4, or bracketed IPv6,
        // with optional port and path/query/fragment.
        static const std::regex withoutScheme(
            R"(^(?:(?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}|localhost|(?:\d{1,3}\.){3}\d{1,3}|\[[0-9a-fA-F:]+\])(?::\d{1,5})?(?:[/?#].*)?$)",
            std::regex::icase);

        return std::regex_match(input, withoutScheme);
    }

    Parts parse(const std::string &url)
    {
        Parts result;

        auto schemeEnd = url.find("://");
        size_t hostStart = 0;

        if (schemeEnd != std::string::npos)
        {
            result.scheme = url.substr(0, schemeEnd);
            hostStart = schemeEnd + 3;
        }

        // Host ends at the first '/', '?' or '#'
        auto hostEnd = url.find_first_of("/?#", hostStart);

        if (hostEnd == std::string::npos)
        {
            result.host = url.substr(hostStart);
            return result;
        }

        result.host = url.substr(hostStart, hostEnd - hostStart);

        // Path
        auto pathEnd = url.find_first_of("?#", hostEnd);

        if (pathEnd == std::string::npos)
        {
            result.path = url.substr(hostEnd);
            return result;
        }

        result.path = url.substr(hostEnd, pathEnd - hostEnd);

        // Query
        if (url[pathEnd] == '?')
        {
            auto fragmentStart = url.find('#', pathEnd + 1);

            if (fragmentStart == std::string::npos)
            {
                result.query = url.substr(pathEnd + 1);
                return result;
            }

            result.query = url.substr(
                pathEnd + 1,
                fragmentStart - pathEnd - 1);

            result.fragment = url.substr(fragmentStart + 1);
            return result;
        }

        // Fragment directly after path
        result.fragment = url.substr(pathEnd + 1);

        return result;
    }

    std::string discardFragment(const Parts &parts)
    {
        std::string url;

        if (!parts.scheme.empty())
            url += parts.scheme + "://";

        url += parts.host;
        url += parts.path;

        if (!parts.query.empty())
            url += "?" + parts.query;

        return url;
    }
}
