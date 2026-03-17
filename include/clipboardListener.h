#pragma once

#include <windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ClipboardListener {
public:
    ClipboardListener() = default;
    ~ClipboardListener();

    void start();
    std::string waitForChange();
    void stop();

private:
    void threadMain();
    void handleClipboard();

    HWND hwnd_{ nullptr };
    DWORD thread_id_{ 0 };

    std::thread thread_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::string latest_;

    std::atomic<bool> running_{ false };
    std::atomic<bool> armed_{ false };
};