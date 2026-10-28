#include "jobHistory.h"

#include <fstream>
#include <algorithm>
#include <queue>

void JobHistory::loadLogs()
{
    std::queue<std::filesystem::path> q;
    q.push(logPath);

    while (!q.empty())
    {
        auto parent = q.front();
        q.pop();

        for (const auto &entry : std::filesystem::directory_iterator(parent))
        {
            if (std::filesystem::is_directory(entry))
                q.push(entry);

            else if (std::filesystem::is_regular_file(entry))
            {
                // std::cout << entry.path().string() << "  " << parent.stem() << "\n";

                // like ig, yt
                std::string serviceKey = parent.stem().string();

                if (parent.parent_path().stem() != "failed" &&
                    parent.parent_path().stem() != "succeeded")
                    throw std::runtime_error("Logs are corrupted, backup and delete logs folder.");

                std::ifstream file(entry.path());
                if (!file)
                    throw std::runtime_error("Cannot open file: " + entry.path().string());

                auto &state = proccessedIds[serviceKey];

                if (state.logFile.empty() ||
                    state.logFile.filename() < entry.path().filename())
                {
                    state.logFile = entry.path();
                }

                std::string id;
                while (std::getline(file, id))
                {
                    if (!id.empty() && id.back() == '\r')
                        id.pop_back();

                    id.erase(
                        id.begin(),
                        std::find_if(id.begin(), id.end(),
                                     [](unsigned char ch)
                                     {
                                         return !std::isspace(ch);
                                     }));

                    id.erase(
                        std::find_if(id.rbegin(), id.rend(),
                                     [](unsigned char ch)
                                     {
                                         return !std::isspace(ch);
                                     })
                            .base(),
                        id.end());

                    if (id.empty())
                        continue;

                    state.ids.emplace(id);
                }
            }
        }
    }
}

bool JobHistory::contains(const std::string &serviceKey,
                          const std::string &id) const
{
    auto itr = proccessedIds.find(serviceKey);
    if (itr != proccessedIds.end())
        return itr->second.ids.count(id);

    return false;
}

void JobHistory::recordLog(const std::string &serviceKey,
                           const std::string &id, LogState state)
{

    // 1. Determine directory
    std::filesystem::path path =
        (state == LogState::Failed) ? logPath / "failed"
                                    : logPath / "succeeded";

    path /= serviceKey;

    // 2. Make sure the directory exists
    std::filesystem::create_directories(path);

    // 3. Find existing service entry, or create one
    auto [itr, inserted] = proccessedIds.try_emplace(serviceKey);

    LogData &logData = itr->second;

    // 4. First log file for this service
    if (inserted)
        logData.logFile = path / "001.log";

    else
        logData.logFile = path / logData.logFile.filename();

    // 5. Rotate if current file is too large
    if (std::filesystem::exists(logData.logFile) &&
        std::filesystem::file_size(logData.logFile) > 100*1024)
    {
        std::string filename = logData.logFile.stem().string();
        int number = std::stoi(filename);

        ++number;

        std::ostringstream nextFile;
        nextFile << std::setw(3)
                 << std::setfill('0')
                 << number
                 << ".log";

        logData.logFile = path / nextFile.str();
    }

    // 6. Record ID
    logData.ids.emplace(id);

    // 7. Write to current log
    std::ofstream file(logData.logFile, std::ios::app);

    if (!file)
        throw std::runtime_error("Cannot open file: " + logData.logFile.string());

    file << id << '\n';
}

void JobHistory::recordSuccess(const std::string &serviceKey, const std::string &id)
{
    recordLog(serviceKey, id, LogState::Succeeded);
}

void JobHistory::recordFailure(const std::string &serviceKey, const std::string &id)
{
    recordLog(serviceKey, id, LogState::Failed);
}