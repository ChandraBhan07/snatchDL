#include "config.h"

namespace fs = std::filesystem;

void Config::handle_config(const fs::path &basePath, ConfigData &configData)
{
    data = configData;
    fs::path configPath = basePath / "config.json";

    if (fs::exists(configPath))
    {
        FileManager::instance().ensureReadable(configPath.string());

        std::ifstream file(configPath);
        std::string line;

        while (std::getline(file, line))
        {
            auto pos = line.find(":");
            if (pos == std::string::npos)
                continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // clean key and value
            key.erase(remove_if(key.begin(), key.end(), isspace), key.end());
            value.erase(remove_if(value.begin(), value.end(), [](char c)
                                  { return c == ' ' || c == '\"' || c == ',' || c == '$'; }),
                        value.end());

            // assign only known keys, ignore unknown
            if (key == "\"logs\"")
                data.paths.logs = value;
            else if (key == "\"downloads\"")
                data.paths.downloads = value;
            else if (key == "\"cookie\"")
                data.paths.cookie = value;
            else if (key == "\"yt_dlp\"")
                data.paths.yt_dlp = value;
            else if (key == "\"max_retries\"")
                data.jobs.max_retries = std::stoi(value);
            else if (key == "\"worker_threads\"")
                data.jobs.worker_threads = std::stoi(value);
            else if (key == "\"initial_pool_per_source\"")
                data.cookies.initial_pool_per_source = std::stoi(value);
        }
    }
    create_config(configPath.string(), data);
}

void Config::create_config(const std::filesystem::path &path, const ConfigData &configData)
{
    std::ofstream out(path, std::ios::trunc);

    out << "{\n";
    out << "  \"paths\": {\n";
    out << "    \"logs\": \"" << configData.paths.logs << "\",\n";
    out << "    \"downloads\": \"" << configData.paths.downloads << "\",\n";
    out << "    \"cookie\": \"" << configData.paths.cookie << "\",\n";
    out << "    \"yt_dlp\": \"" << configData.paths.yt_dlp << "\"\n";
    out << "  },\n";

    out << "  \"jobs\": {\n";
    out << "    \"max_retries\": " << configData.jobs.max_retries << ",\n";
    out << "    \"worker_threads\": " << configData.jobs.worker_threads << "\n";
    out << "  },\n";

    out << "  \"cookies\": {\n";
    out << "    \"initial_pool_per_source\": " << configData.cookies.initial_pool_per_source << "\n";
    out << "  }\n";

    out << "}\n";

    out.close();
}
