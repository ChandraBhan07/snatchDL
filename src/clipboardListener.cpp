#include "ClipboardListener.h"

ClipboardListener::~ClipboardListener()
{
    stop();

    if (thread_.joinable())
        thread_.join();
}

void ClipboardListener::start()
{
    if (running_)
        return;

    running_ = true;
    armed_.store(true, std::memory_order_release);

    thread_ = std::thread(&ClipboardListener::threadMain, this);
}

void ClipboardListener::stop()
{
    running_ = false;

    if (thread_id_)
        PostThreadMessage(thread_id_, WM_QUIT, 0, 0);

    cv_.notify_all();
}

void ClipboardListener::threadMain()
{
    thread_id_ = GetCurrentThreadId();

    static constexpr wchar_t CLASS_NAME[] = L"ClipboardListenerWindow";

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)->LRESULT
    {
        if (msg == WM_CREATE)
        {
            auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return 0;
        }

        if (msg == WM_CLIPBOARDUPDATE)
        {
            auto self = reinterpret_cast<ClipboardListener*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));

            if (self)
                self->handleClipboard();

            return 0;
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    };

    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        0,
        CLASS_NAME,
        L"",
        0,
        0,0,0,0,
        HWND_MESSAGE,
        nullptr,
        wc.hInstance,
        this);

    if (!hwnd_)
        return;

    AddClipboardFormatListener(hwnd_);

    MSG msg;

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    RemoveClipboardFormatListener(hwnd_);
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
}

void ClipboardListener::handleClipboard()
{
    if (!armed_.load(std::memory_order_acquire))
        return;

    if (!OpenClipboard(nullptr))
        return;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);

    if (hData)
    {
        auto wtext = static_cast<wchar_t*>(GlobalLock(hData));

        if (wtext)
        {
            int size = WideCharToMultiByte(
                CP_UTF8, 0, wtext, -1, nullptr, 0, nullptr, nullptr);

            std::string text(size - 1, '\0');

            WideCharToMultiByte(
                CP_UTF8, 0, wtext, -1, text.data(), size, nullptr, nullptr);

            {
                std::lock_guard<std::mutex> lock(mutex_);
                latest_ = std::move(text);
            }

            cv_.notify_all();

            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
}

std::string ClipboardListener::waitForChange()
{
    std::unique_lock<std::mutex> lock(mutex_);

    cv_.wait(lock, [this]
    {
        return !running_ || !latest_.empty();
    });

    if (!running_)
        return {};

    std::string result = std::move(latest_);
    latest_.clear();

    return result;
}