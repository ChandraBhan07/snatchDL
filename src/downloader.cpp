#include "downloader.h"

Downloader::Downloader(const std::filesystem::path &downloadDir,
                       const std::filesystem::path &ytDlpExe)
    : downloadRoot(downloadDir),
      ytDlpPath(ytDlpExe)
{
    initDir();
    validateYtDlp();
}

void Downloader::initDir() const
{
    FileManager::instance().ensureDirectory(downloadRoot);
}

void Downloader::validateYtDlp()
{
    FileManager::instance().ensureParentDirectory(ytDlpPath);
    if (!FileManager::instance().exists(ytDlpPath.string()))
    {
        std::wstring cmd = L"yt-dlp --version";
        
        // /If exit code == 0 → found globally else → not found
        DWORD res = excecuter.run(cmd);
        if (res != 0)
        {
            throw std::runtime_error("Either place yt-dlp into local directory or add it to environment variable.");
        }
        else {
            std::cout << "Found global yt-dlp executable. Proceeding ...\n";
            // needs improvement
            ytDlpPath = "yt-dlp.exe";
        }
    }
    else std::cout << "Found local yt-dlp executable. Proceeding ...\n";
}


// Marked const so it can be called from const methods
std::wstring Downloader::utf8ToWstring(const std::string &s) const
{
    if (s.empty())
        return {};

    int size = MultiByteToWideChar(
        CP_UTF8,
        0,
        s.data(),
        static_cast<int>(s.size()),
        nullptr,
        0);

    std::wstring result(size, 0);

    MultiByteToWideChar(
        CP_UTF8,
        0,
        s.data(),
        static_cast<int>(s.size()),
        result.data(),
        size);

    return result;
}

std::wstring Downloader::buildCommand(
    const std::filesystem::path &outDir,
    const std::wstring &url,
    const Cookie *cookie) const
{
    std::wstring cmd = L"\"" + ytDlpPath.wstring() + L"\" ";

    if (cookie)
    {
        cmd += L"--cookies \"" + cookie->path.wstring() + L"\" ";
    }
    cmd += L"--merge-output-format mp4 ";
    cmd += L"-o \"" + (outDir / L"%(channel)s %(id)s.%(ext)s").wstring() + L"\" ";
    cmd += url;

    return cmd;
}

std::string wstringToUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), strTo.data(), size_needed, nullptr, nullptr);
    return strTo;
}

DWORD Downloader::processLink(
    const std::string &id,
    const std::string &source,
    const Cookie *cookie)
{

    std::string url =  Source::instance().getCanonicalForm(source) + "/" + id;
    std::wstring wUrl = utf8ToWstring(url);

    std::filesystem::path outDir = downloadRoot / source;
    std::filesystem::create_directories(outDir);

    std::wstring cmd = buildCommand(outDir, wUrl, cookie);
    
    return excecuter.run(cmd);
}
