#pragma once

#include "../config/config.h"

#include <string>

class CliOptions
{
    // private:
public:
    Config config;

public:
    ErrorState errorState = ErrorState::None;
    static CliOptions parse(int argc, char *argv[]);

    static void printHelp();
    static void printVersion();

    bool showHelp = false;
    bool showVersion = false;

    std::string errorMessage;
};