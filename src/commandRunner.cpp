#include <commandRunner.h>

DWORD CommandRunner::run(std::wstring &cmd)
{

    // --------------------
    // Create pipe
    // --------------------
    HANDLE hStdOutRead = nullptr;
    HANDLE hStdOutWrite = nullptr;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE; // child can inherit
    sa.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0))
        throw std::runtime_error("CreatePipe failed");

    // Parent must NOT inherit the read handle
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    // --------------------
    // Startup info
    // --------------------
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite; // merge stderr
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};

    // --------------------
    // Create process
    // --------------------
    if (!CreateProcessW(
            nullptr,
            cmd.data(), // MUST be mutable
            nullptr,
            nullptr,
            TRUE, // ✅ allow handle inheritance
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi))
    {
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        throw std::runtime_error("Failed to launch yt-dlp");
    }

    // Parent must close write end immediately
    CloseHandle(hStdOutWrite);
    hStdOutWrite = nullptr;

    // --------------------
    // Read output
    // --------------------
    std::string output;
    char buffer[4096];
    DWORD bytesRead = 0;

    std::cout << "\n";
    while (true)
    {
        BOOL success = ReadFile(
            hStdOutRead,
            buffer,
            sizeof(buffer) - 1,
            &bytesRead,
            nullptr);

        if (!success || bytesRead == 0)
            break;

        buffer[bytesRead] = '\0';
        output.append(buffer, bytesRead);

        // live streaming (optional)
        std::cout << buffer;
    }

    std::cout << "\n";

    // --------------------
    // Wait + exit code
    // --------------------
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // --------------------
    // Cleanup
    // --------------------
    CloseHandle(hStdOutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode;
}