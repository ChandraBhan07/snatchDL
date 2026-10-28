#include "config.h"

Config::Config() : batchSize(0),
                   mode(RunMode::Default),
                   eachDelay(0),
                   startDelay(0),
                   batchDelay(0)
{
    exePath = getExeDir();

    ytDlpDir = exePath / "data" / "others";
    cookiesDir = exePath / "data" / "cookies";
    downloadDir = exePath / "data" / "downloads";
    logsDir = exePath / "data" / "logs";

    std::filesystem::create_directories(ytDlpDir);
    std::filesystem::create_directories(cookiesDir);
    std::filesystem::create_directories(downloadDir);
    std::filesystem::create_directories(logsDir);
    std::filesystem::create_directories(logsDir / "succeeded");
    std::filesystem::create_directories(logsDir / "failed");
}

FinalizeResult Config::finalize() const
{
    FinalizeResult result;

    if (inputFile.empty())
    {
        result.state = ErrorState::Fatal;
        result.message = "Please specify input file.";
        return result;
    }

    if (mode == RunMode::ListenOnly && batchSize != 0)
    {
        result.state = ErrorState::Warning;
        result.message.append("INFO: --batch has no effect in --listen-only mode.\n");
    }

    if (mode == RunMode::ListenOnly && eachDelay.count() != 0)
    {
        result.state = ErrorState::Warning;
        result.message.append("INFO: --each-delay has no effect in --listen-only mode.\n");
    }

    if (mode == RunMode::ListenOnly && startDelay.count() != 0)
    {
        result.state = ErrorState::Warning;
        result.message.append("INFO: --start-delay has no effect in --listen-only mode.\n");
    }

    if (mode == RunMode::ListenOnly && !limitRate.empty())
    {
        result.state = ErrorState::Warning;
        result.message.append("INFO: --limit-rate has no effect in --listen-only mode.\n");
    }

    return result;
}

void Config::expireStartDelay() const
{
    if (startDelay.count())
        startDelay = std::chrono::seconds{0};
}