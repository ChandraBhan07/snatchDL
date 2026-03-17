#pragma once
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>

#include "link.h"
#include "source.h"
#include "fileManager.h"

namespace fs = std::filesystem;

class Parser
{
public:
    Parser() {};
    void load_logs(const fs::path &dir, std::unordered_set<std::string> &seen_id);

    bool is_text_file(const std::string &path);
    void validate_input_file(const std::string& path);

    std::vector<Link> compute(const std::string &path, std::unordered_set<std::string> &seen_id);
    void normalize_text(std::string &line);
    bool isHttpsUrl(const std::string &text);
    bool addLinkIfNew(std::string &text, std::unordered_set<std::string> &seen_id, std::vector<Link> &pending);
    void appendToFile(const std::string url, const std::string &path);
};