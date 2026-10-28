#pragma once
#include <string>

namespace ProcessRunner
{
    int run(const std::string &command, bool printStream = false);
}