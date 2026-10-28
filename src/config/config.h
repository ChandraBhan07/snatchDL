#include <filesystem>
#include <chrono>
#include <string>

enum class RunMode
{
    Default,
    ListenOnly,
    DownloadOnly
};

enum class ErrorState
{
    None,
    Fatal,
    Warning
};

struct FinalizeResult
{
    ErrorState state = ErrorState::None;
    std::string message;
};

struct Config
{
    std::filesystem::path inputFile;
    RunMode mode;
    int batchSize;
    std::chrono::seconds eachDelay;
    mutable std::chrono::seconds startDelay;
    std::chrono::seconds batchDelay;
    std::string limitRate;

    std::filesystem::path exePath;
    std::filesystem::path ytDlpDir;
    std::filesystem::path cookiesDir;
    std::filesystem::path downloadDir;
    std::filesystem::path logsDir;

    std::string errorMessage;

    FinalizeResult finalize() const;
    std::filesystem::path getExeDir();

    void expireStartDelay() const;
    Config();
};
