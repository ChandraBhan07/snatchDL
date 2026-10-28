#pragma once

#include "../job/job.h"

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

template <typename t>
class ThreadSafeQueue;

struct Config;

class CookieManager;

class Downloader
{
    struct JobCompare
    {
        bool operator()(const Job &a, Job &b) const
        {
            return a.readyAt > b.readyAt;
        }
    };
    enum class YtDlpSourceType
    {
        Local,
        System
    };

    ThreadSafeQueue<Job> &incomingQueue;
    ThreadSafeQueue<Job> &outgoingQueue;
    std::atomic<bool> running{true};
    std::mutex mtx;
    std::condition_variable cv;
    YtDlpSourceType ytDlpSourceType;

    const Config &config;
    CookieManager &cookieManager;

public:
    Downloader(ThreadSafeQueue<Job> &incoming,
               ThreadSafeQueue<Job> &outgoing,
               const Config &config,
               CookieManager &cManager) : incomingQueue(incoming),
                                          outgoingQueue(outgoing),
                                          config(config),
                                          cookieManager(cManager) {};

    void init();
    void run();
    void stop();

    std::priority_queue<Job, std::vector<Job>, JobCompare> queue;
};