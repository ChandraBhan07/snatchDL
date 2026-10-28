#include "../listener.h"
#include "../../runtime/threadSafeQueue.h"

#include <windows.h>

#include <iostream>

namespace
{
    std::atomic<DWORD> listenerThreadId{0};
}

void Listener::run()
{

    // Force creation of this thread's message queue
    MSG msg{};

    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    listenerThreadId.store(GetCurrentThreadId());

    WNDCLASSEXW wc{};

    wc.cbSize = sizeof(WNDCLASSEXW);

    wc.lpfnWndProc = [](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT CALLBACK
    {
        if (message == WM_NCCREATE)
        {
            auto *createStruct = reinterpret_cast<CREATESTRUCTW *>(lParam);

            auto *listener = static_cast<Listener *>(createStruct->lpCreateParams);

            SetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(listener));

            return true;
        }

        if (message == WM_CLIPBOARDUPDATE)
        {
            auto *listener =
                reinterpret_cast<Listener *>(
                    GetWindowLongPtrW(hwnd, GWLP_USERDATA));

            if (OpenClipboard(hwnd))
            {
                if (IsClipboardFormatAvailable(CF_UNICODETEXT))
                {
                    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
                    wchar_t *text = static_cast<wchar_t *>(GlobalLock(handle));
                    std::wstring wide(text);

                    int size = WideCharToMultiByte(
                        CP_UTF8,
                        0,
                        wide.data(),
                        static_cast<int>(wide.size()),
                        nullptr,
                        0,
                        nullptr,
                        nullptr);

                    std::string result(size, '\0');

                    WideCharToMultiByte(
                        CP_UTF8,
                        0,
                        wide.data(),
                        static_cast<int>(wide.size()),
                        result.data(),
                        size,
                        nullptr,
                        nullptr);

                    listener->queue.enqueue(result);

                    GlobalUnlock(handle);
                }
                else
                {
                    std::cout << "IsClipboardFormatAvailable failed: " << GetLastError() << "\n";
                }
                CloseClipboard();
            }
            else
            {
                std::cout << "OpenClipboard failed: " << GetLastError() << "\n";
            }
        }

        return DefWindowProc(hwnd, message, wParam, lParam);
    };

    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"ClipboardListener";

    RegisterClassExW(&wc);

    // remember the last this parameter
    HWND hwnd = CreateWindowExW(0, L"ClipboardListener", L"ClipboardListener", 0,
                                0, 0, 0, 0, nullptr, nullptr, wc.hInstance, this);

    AddClipboardFormatListener(hwnd);

    std::cout <<"Listening to clipboard ...\n";

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    RemoveClipboardFormatListener(hwnd);
    DestroyWindow(hwnd);
    UnregisterClassW(L"ClipboardListener", GetModuleHandleW(nullptr));

    listenerThreadId.store(0);
}

void Listener::stop()
{
    DWORD id = listenerThreadId.load();

    if (id != 0)
    {
        PostThreadMessageW(id, WM_QUIT, 0, 0);
    }
}