#include "../config.h"
#include <windows.h>
#include <filesystem>

std::filesystem::path Config::getExeDir()
{
    // Windows-specific implementation
    wchar_t buffer[MAX_PATH];

    DWORD length = GetModuleFileNameW(
        nullptr,
        buffer,
        MAX_PATH);

    return std::filesystem::path(buffer).parent_path();
}

