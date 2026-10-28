#include "../processRunner.h"

#include <windows.h>
#include <iostream>

namespace ProcessRunner
{
    int run(const std::string &command, bool printStream)
    {

        // step1: create empty handles
        // step2: not anonymous but inheritable pipe requires security attributes then create pipe
        // step3: we will not inherit write side of pipe to child
        // step4: startup info for the child process aka environment for child
        // step5: create empty process information for knowing exit code
        // step6: createprocessW with widing string to wstring
        // step7: closing parent's write handle so when child finishes writing there's no writer left
        // step8: readfile with the handle
        // step9: waitforsingleobject and then getexitcodeprocess

        // step 1
        HANDLE readHandle = nullptr;
        HANDLE writeHandle = nullptr;

        // step 2
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = true; // important
        sa.lpSecurityDescriptor = nullptr;

        if (!CreatePipe(&readHandle, &writeHandle, &sa, 0))
        {
            std::cerr << "Create Pipe failed with: " << GetLastError() << "\n";
            return 1;
        }

        // step 3
        SetHandleInformation(
            readHandle,
            HANDLE_FLAG_INHERIT,
            0);

        // step 4

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = writeHandle;
        si.hStdError = writeHandle;

        // step 5
        PROCESS_INFORMATION pi{};

        std::wstring wideCommand(command.begin(), command.end());

        // DWORD creationFlags = 0;
        DWORD creationFlags = CREATE_NO_WINDOW;

        // step 6
        if (!CreateProcessW(
                nullptr,            // let command line identify program
                wideCommand.data(), // what to launch
                nullptr,            // default process security
                nullptr,            // default thread security
                TRUE,               // allow inheritable handles
                creationFlags,      // no special creation flags
                nullptr,            // parent's environment
                nullptr,            // parent's current directory
                &si,                // startup configuration
                &pi                 // give me process/thread information
                ))
        {
            CloseHandle(readHandle);
            CloseHandle(writeHandle);
            std::cerr << "Create process failed with " << GetLastError() << "\n";
            return 1;
        }
        // step 7
        CloseHandle(writeHandle);
        writeHandle = nullptr;

        // step 8
        char buffer[4096];
        DWORD bytesRead = 0;

        while (true)
        {
            BOOL success = ReadFile(
                readHandle,
                buffer,
                sizeof(buffer) - 1,
                &bytesRead,
                nullptr);

            if (!success || bytesRead == 0)
                break;

            buffer[bytesRead] = '\0';
            if (printStream)
                std::cout << buffer;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(readHandle);
        CloseHandle(writeHandle);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return static_cast<int>(exitCode);
    }
}