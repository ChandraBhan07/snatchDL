#include "../shutdown.h"

#include <windows.h>

namespace Shutdown
{
    HANDLE g_shutdownEvent = nullptr;

    BOOL WINAPI shutdownHandler(DWORD dwCtrlType)
    {
        if (dwCtrlType == CTRL_C_EVENT)
        {
            SetEvent(g_shutdownEvent);
            return TRUE;
        }
        return FALSE;
    }

    void initialize()
    {
        // Shutdown
        // g_shutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        // SetConsoleCtrlHandler(shutdownHandler, TRUE);

        Shutdown::g_shutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        SetConsoleCtrlHandler(Shutdown::shutdownHandler, TRUE);
    }

    void awaitShutdown()
    {
        // Block until Ctrl+C
        WaitForSingleObject(Shutdown::g_shutdownEvent, INFINITE);

        SetConsoleCtrlHandler(Shutdown::shutdownHandler, FALSE);
        CloseHandle(Shutdown::g_shutdownEvent);
    }

}