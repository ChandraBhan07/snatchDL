#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>

class Source
{
public:
    static Source &instance();

    // main api
    std::pair<std::string, std::string> resolveUrl(const std::string &url);
    std::string normalizeDomain(const std::string &domain);
    std::string getSource(const std::string &host);
    std::string getCanonicalForm(const std::string &source);

private:
    Source();
    Source(const Source &) = delete;
    Source &operator=(const Source &) = delete;

    std::string extract_host(const std::string &url) const;
    std::string normalize_host(std::string host) const;

    std::string extract_path(const std::string &url) const;
    std::string extract_query(const std::string &url) const;
    std::string get_query_param(const std::string &query,
                                const std::string &key) const;

    std::unordered_map<std::string, std::string> host_to_source;

    // canonical form is used to construct downloading link by id
    std::unordered_map<std::string, std::string>cnn_forms;
};
