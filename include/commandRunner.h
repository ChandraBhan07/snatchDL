#pragma once
#include <windows.h>
#include <string>
#include <iostream>

class CommandRunner {
public:
    DWORD run(std::wstring& cmd);
    // Future: run(std::string& cmd) for Unix paths
    // Future: run(const std::vector<std::string>& args)
};