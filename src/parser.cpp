#include "parser.h"

void Parser::load_logs(const fs::path &dir, std::unordered_set<std::string> &seen_id)
{

    FileManager::instance().ensureDirectory(dir);

    for (const auto &entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file())
            continue;

        fs::path p = entry.path();

        // "ig.logs" -> "ig"
        std::string source = p.stem().string();

        std::ifstream file(p);
        if (!file)
            throw std::runtime_error("Cannot open file: " + p.string());

        std::string id;
        while (std::getline(file, id))
        {
            normalize_text(id);

            if (id.empty())
                continue;

            std::string key = source + ":" + id;

            if (!seen_id.count(key))
                seen_id.insert(key);
        }
    }
}

void Parser::normalize_text(std::string &line)
{
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    line.erase(line.begin(), std::find_if(line.begin(), line.end(),
                                          [](unsigned char ch)
                                          { return !std::isspace(ch); }));
    line.erase(std::find_if(line.rbegin(), line.rend(),
                            [](unsigned char ch)
                            { return !std::isspace(ch); })
                   .base(),
               line.end());
}

std::vector<Link> Parser::compute(const std::string &path,
                                  std::unordered_set<std::string> &seen_id)
{
    FileManager::instance().ensureReadable(path);
    std::ifstream file(path);

    std::vector<Link> pending;
    std::string line;

    while (std::getline(file, line))
    {
        addLinkIfNew(line, seen_id, pending);
    }

    return pending;
}

bool Parser::addLinkIfNew(std::string &text, std::unordered_set<std::string> &seen_id, std::vector<Link> &pending)
{
    normalize_text(text);
    if (text.empty())
        return false;

    auto [source, id] = Source::instance().resolveUrl(text);
    if (id.empty())
        return false;

    std::string key = source + ":" + id;
    if (seen_id.count(key))
        return false;

    seen_id.insert(key);
    pending.emplace_back(id, source);
    return true;
}

bool Parser::is_text_file(const std::string &path)
{
    FileManager::instance().ensureReadable(path);
    std::ifstream file(path, std::ios::binary);

    char ch;
    int count = 0;
    while (file.get(ch) && count < 1024)
    {
        if (ch == '\0')
            throw std::runtime_error("File '" + path + "' is not a text file");
        if (!isprint(static_cast<unsigned char>(ch)) && !isspace(static_cast<unsigned char>(ch)))
            throw std::runtime_error("File '" + path + "' is not a text file");
        ++count;
    }
    return true;
}

void Parser::validate_input_file(const std::string &path)
{
    // Step 1: Validate file exists
    FileManager::instance().ensureReadable(path);

    // Step 2: Validate it's text (first 1KB check)
    if (!is_text_file(path))
        throw std::runtime_error("File '" + path + "' is not a text file");
}

bool Parser::isHttpsUrl(const std::string &text)
{
    const std::string prefix = "https://";
    return text.rfind(prefix, 0) == 0; // starts with
}

void Parser::appendToFile(const std::string url, const std::string &path)
{
    FileManager::instance().ensureWritable(path);
    FileManager::instance().write(path, url);
}