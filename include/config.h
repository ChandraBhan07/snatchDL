#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

#include "fileManager.h"

struct Paths
{
    std::string logs;
    std::string downloads;
    std::string cookie;
    std::string yt_dlp;
};

struct Jobs
{
    int max_retries;
    int worker_threads;
};

struct Cookies
{
    int initial_pool_per_source;
};

struct ConfigData
{
    Paths paths;
    Jobs jobs;
    Cookies cookies;
};

class Config
{
    void create_config(const std::filesystem::path &path, const ConfigData &configData);

public:
    ConfigData data;
    void handle_config(const std::filesystem::path &basePath, ConfigData &ConfigData);
};
