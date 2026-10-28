#pragma once
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <optional>

template <typename T>
class ThreadSafeQueue
{
private:
    std::mutex mtx;
    std::condition_variable cv;
    std::deque<T> queue;
    std::atomic<bool> stopping{false};

public:
    void enqueue(T item)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push_back(std::move(item));
        }

        cv.notify_one();
    }

    bool wait()
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [&]
                { return stopping.load() || !queue.empty(); });

        return !stopping.load();
    }

    std::optional<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (queue.empty())
            return std::nullopt;

        T item = std::move(queue.front());
        queue.pop_front();

        return item;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }

    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stopping.store(true);
        }

        cv.notify_all();
    }
};
