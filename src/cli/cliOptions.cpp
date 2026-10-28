#include "cliOptions.h"

#include <iostream>

bool checkAndLimit(std::string &errorMessage, int maxParameterSize, const std::string &argument, const std::string &parameter)
{
    std::string str(maxParameterSize, '9');

    // remove leading zeroes from the parameter like 00000001
    // we track and calculate substr

    // bool pos
    std::pair<bool, int> firstNumberFound{false, 0};

    for (int itr = 0; itr < parameter.size(); ++itr)
    {
        char c = parameter[itr];

        if (c < '0' || c > '9')
        {
            errorMessage = argument + " requires a integer from 0 to " + str + ".";
            return false;
        }

        if (firstNumberFound.first == false && c - '0' > 0)
        {
            firstNumberFound.second = itr;
        }
    }

    if (parameter.size() - firstNumberFound.second > maxParameterSize)
    {
        errorMessage = argument + " must be between 0 and " + str + ".";
        return false;
    }
    return true;
}

bool filterAndParse(std::string &errorMessage, std::string rate)
{
    char front = rate.front();

    if (front == '.' || front < '0' || front > '9')
    {
        errorMessage = "Invalid rate, see help or -h for correct types.";
        return false;
    }

    char unit = rate.back();
    if (unit == 'G' || unit == 'g' || unit == 'M' || unit == 'm' || unit == 'K' || unit == 'k')
    {
        rate.pop_back();
    }

    // eliminates random last characters and 4.G or 4. type inputs
    if (rate.empty() || rate.back() < '0' || rate.back() > '9')
    {
        errorMessage = "Invalid rate, see help or -h for correct types.";
        return false;
    }

    int count = 0;
    for (char ch : rate)
    {
        if (ch == '.')
        {
            if (count > 0)
            {
                errorMessage = "Invalid rate, see help or -h for correct types.";
                return false;
            }
            count++;
            continue;
        }
        else if (ch < '0' || ch > '9')
        {
            errorMessage = "Invalid rate, see help or -h for correct types.";
            return false;
        }
    }

    if (std::stof(rate) <= 0)
    {
        errorMessage = "Enter positive rate, see help or -h for correct types.";
        return false;
    }

    return true;
}

CliOptions CliOptions::parse(int argc, char *argv[])
{
    CliOptions options;

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];

        if (argument == "-h" || argument == "-H" || argument == "--help")
        {
            options.showHelp = true;
        }
        else if (argument == "-v" || argument == "-V" || argument == "--version")
        {
            options.showVersion = true;
        }

        else if (argument == "--listen-only")
        {
            if (options.config.mode == RunMode::DownloadOnly)
            {
                options.errorState = ErrorState::Fatal;
                options.errorMessage = "--listen-only and --download-only cannot be used together.";
                return options;
            }
            if (options.config.mode == RunMode::ListenOnly)
            {
                options.errorState = ErrorState::Fatal;
                options.errorMessage = "--listen-only may only be specified once.";
                return options;
            }
            options.config.mode = RunMode::ListenOnly;
        }

        else if (argument == "--download-only")
        {
            if (options.config.mode == RunMode::ListenOnly)
            {
                options.errorState = ErrorState::Fatal;
                options.errorMessage = "--listen-only and --download-only cannot be used together.";
                return options;
            }
            if (options.config.mode == RunMode::DownloadOnly)
            {
                options.errorState = ErrorState::Fatal;
                options.errorMessage = "--download-only may only be specified once.";
                return options;
            }
            options.config.mode = RunMode::DownloadOnly;
        }

        else if (argument.substr(0, 1) == "-" || argument.substr(0, 2) == "--")
        {

            if (argument == "--batch")
            {
                if (options.config.batchSize)
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--batch may be specified once.";
                    return options;
                }

                if (i + 1 >= argc)
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--batch requires an integer count next to it.";
                    return options;
                }

                const std::string parameter = argv[i + 1];

                if (checkAndLimit(options.errorMessage, 2, argument, parameter) == false)
                {
                    options.errorState = ErrorState::Fatal;
                    return options;
                }

                options.config.batchSize = std::stoi(parameter);

                // consuming next parameter
                ++i;
            }

            else if (argument == "--each-delay")
            {
                if (options.config.eachDelay.count())
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--each-delay may be specified once.";
                    return options;
                }

                if (i + 1 >= argc)
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--each-delay requires an integer count next to it.";
                    return options;
                }

                const std::string parameter = argv[i + 1];

                if (checkAndLimit(options.errorMessage, 4, argument, parameter) == false)
                {
                    options.errorState = ErrorState::Fatal;
                    return options;
                }

                std::chrono::seconds sec{std::stoi(parameter)};
                options.config.eachDelay = sec;

                ++i;
            }

            else if (argument == "--start-delay")
            {
                if (options.config.startDelay.count())
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--start-delay may be specified once.";
                    return options;
                }

                if (i + 1 >= argc)
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--start-delay requires an integer count next to it.";
                    return options;
                }

                const std::string parameter = argv[i + 1];

                if (checkAndLimit(options.errorMessage, 4, argument, parameter) == false)
                {
                    options.errorState = ErrorState::Fatal;
                    return options;
                }

                std::chrono::seconds sec{std::stoi(parameter)};
                options.config.startDelay = sec;

                ++i;
            }

            else if (argument == "--batch-delay")
            {
                if (options.config.batchDelay.count())
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--batch-delay may be specified once.";
                    return options;
                }

                if (i + 1 >= argc)
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--batch-delay requires an integer count next to it.";
                    return options;
                }

                const std::string parameter = argv[i + 1];

                if (checkAndLimit(options.errorMessage, 4, argument, parameter) == false)
                {
                    options.errorState = ErrorState::Fatal;
                    return options;
                }

                std::chrono::seconds sec{std::stoi(parameter)};
                options.config.batchDelay = sec;

                ++i;
            }

            else if (argument == "--limit-rate")
            {
                if (!options.config.limitRate.empty())
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--limt-rate may be specified once.";
                    return options;
                }

                if (i + 1 >= argc)
                {
                    options.errorState = ErrorState::Fatal;
                    options.errorMessage = "--limit-rate requires value next to it.";
                    return options;
                }

                std::string parameter = argv[i + 1];

                if (filterAndParse(options.errorMessage, parameter) == false)
                {
                    options.errorState = ErrorState::Fatal;
                    return options;
                }

                options.config.limitRate = argv[i + 1];
                ++i;
            }

            else
            {
                options.errorState = ErrorState::Fatal;
                options.errorMessage = "Unknown option: " + std::string(argument);

                return options;
            }
        }
        else
        {

            // input filename
            if (!options.config.inputFile.empty())
            {
                options.errorState = ErrorState::Fatal;
                options.errorMessage = "Only one input file may be specified.";
                return options;
            }
            options.config.inputFile = std::filesystem::absolute(argument);
        }
    }

    return options;
}

void CliOptions::printHelp()
{
    std::cout
        << "SnatchDL - Lightweight download orchestrator\n"
        << '\n'

        << "Usage:\n"
        << "  snatchDL.exe <input-file> [options]\n"
        << '\n'

        << "Modes:\n"
        << "  --listen-only\n"
        << "      Listen for clipboard URLs only.\n"
        << '\n'

        << "  --download-only\n"
        << "      Download links from the input file only.\n"
        << '\n'

        << "Options:\n"
        << "  --batch <count>\n"
        << "      Process at most <count> jobs per downloader cycle.\n"
        << '\n'

        << "  --each-delay <seconds>\n"
        << "      Add an additional delay before each job.\n"
        << '\n'

        << "  --start-delay <seconds>\n"
        << "      Delay at start of downloading only once.\n"
        << '\n'

        << "  --batch-delay <seconds>\n"
        << "      Add an additional delay before each batch.\n"
        << '\n'

        << "  --limit-rate RATE\n"
        << "      Maximum download rate in bytes per second.\n"
        << "      e.g. 50K or 4.2M or 0.4G, add only one letter.\n"
        << '\n'

        << "  -h, --help\n"
        << "      Show this help message.\n"
        << '\n'

        << "  -V, --version\n"
        << "      Show version information.\n"
        << '\n'

        << "Examples:\n"
        << "  snatchDL.exe links.txt\n"
        << "  snatchDL.exe links.txt --listen-only\n"
        << "  snatchDL.exe links.txt --download-only\n"
        << "  snatchDL.exe links.txt --batch 5\n"
        << "  snatchDL.exe links.txt --each-delay 10\n"
        << "  snatchDL.exe links.txt --start-delay 600\n"
        << "  snatchDL.exe links.txt --batch-delay 30\n"
        << "  snatchDL.exe links.txt --limit-rate 500K\n"
        << "  snatchDL.exe links.txt --limit-rate 4M\n"
        << "  snatchDL.exe links.txt --batch 5 --each-delay 10\n\n";
}

void CliOptions::printVersion()
{
    std::cout << "SnatchDL 1.0.0\n";
}