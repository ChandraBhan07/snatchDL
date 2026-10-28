#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <filesystem>

struct Config;

class JobHistory
{
    struct LogData
    {
        std::unordered_set<std::string> ids;
        std::filesystem::path logFile;
    };

    enum struct LogState
    {
        Failed,
        Succeeded
    };

public:
    JobHistory(const std::filesystem::path &path) : logPath(path) {}

    void loadLogs();

    bool contains(const std::string &serviceKey, const std::string &id) const;
    void recordSuccess(const std::string &serviceKey,
                       const std::string &id);

    void recordFailure(const std::string &serviceKey,
                       const std::string &id);

private:
    std::filesystem::path logPath;
    std::unordered_map<std::string, LogData> proccessedIds;
    void recordLog(const std::string &serviceKey,
                   const std::string &id, LogState state);
};