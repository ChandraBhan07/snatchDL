#pragma once
#include <queue>
#include <mutex>
#include <random>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include "job.h"

class JobManager
{
public:
    explicit JobManager(size_t maxRetries, bool listen);
    ~JobManager();
    void shutdown();

    void add_job( Job &job);

    // called by workers
    std::shared_ptr<Job> acquire_job();

    // called by workers
    void mark_success(const std::shared_ptr<Job> &job);
    void mark_failure(const std::shared_ptr<Job> &job);

    bool empty() const;
    size_t get_max_retries() const
    {
        return maxRetries;
    }

private:
    struct JobCompare
    {
        bool operator()(const std::shared_ptr<Job> &a,
                        const std::shared_ptr<Job> &b) const
        {
            return a->ready_at > b->ready_at; // earliest first
        }
    };

    size_t maxRetries;

    mutable std::mutex mtx;
    std::condition_variable cv;

    std::priority_queue<
        std::shared_ptr<Job>,
        std::vector<std::shared_ptr<Job>>,
        JobCompare>
        queue;

    std::chrono::seconds random_delay(const std::string &job_id);
    std::atomic<bool> running{true};
    bool listen;
};
