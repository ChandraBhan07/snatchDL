#pragma once

#include <windows.h>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "fileManager.h"
#include "cookie.h"
#include "commandRunner.h"
#include "source.h"

class Downloader
{
private:
    std::filesystem::path downloadRoot;
    std::filesystem::path ytDlpPath;

    void initDir() const;
    void validateYtDlp() ;

    std::wstring utf8ToWstring(const std::string& s) const;

    CommandRunner excecuter;

public:
    Downloader(const std::filesystem::path& downloadDir,
               const std::filesystem::path& ytDlpExe);

    DWORD processLink(const std::string& id,
                      const std::string& source,
                      const Cookie* cookie);

    std::wstring buildCommand(const std::filesystem::path& outDir,
                              const std::wstring& url,
                              const Cookie* cookie) const;
};