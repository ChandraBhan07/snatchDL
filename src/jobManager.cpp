#include "jobManager.h"

// tmp
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

std::string mm_ss(std::chrono::system_clock::time_point tp)
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << tm.tm_hour
        << "."
        << std::setw(2) << std::setfill('0') << tm.tm_min
        << "."
        << std::setw(2) << std::setfill('0') << tm.tm_sec;

    return oss.str();
}
// tmp

JobManager::JobManager(size_t maxRetries_, bool listen_) : maxRetries(maxRetries_), listen(listen_) {}

// std::chrono::seconds JobManager::random_delay()
// {
//     // One RNG per thread → thread-safe
//     thread_local std::mt19937 rng{std::random_device{}()};

//     // inclusive range
//     std::uniform_int_distribution<int> dist(191, 1196);

//     return std::chrono::seconds(dist(rng));
// }

JobManager::~JobManager()
{
    shutdown();
}

void JobManager::shutdown()
{
    running = false;
    cv.notify_all();
}

std::chrono::seconds JobManager::random_delay(const std::string &job_id)
{
    std::hash<std::string> hasher;
    size_t h = hasher(job_id);

    int range = 596 - 11;
    int offset = static_cast<int>(h % range + 07);

    return std::chrono::seconds(offset);
}

void JobManager::add_job(Job &job)
{
    std::lock_guard<std::mutex> lock(mtx);

    job.ready_at = std::chrono::system_clock::now() + random_delay(job.id) + std::chrono::seconds(30);
    job.state = Job::State::WAITING;

    std::cout << "Job addded " << job.source << " " << job.id << " at " << mm_ss(job.ready_at) << "\n";

    queue.push(std::make_shared<Job>(job));
    cv.notify_one();
}

// std::shared_ptr<Job> JobManager::acquire_job()
// {
//     std::unique_lock<std::mutex> lock(mtx);

//     while (running)
//     {
//         std::cout <<"isme th jara rha h" ;

//         if (!queue.empty())
//         {
//             auto job = queue.top();
//             auto now = std::chrono::system_clock::now();

//             if (job->is_ready())
//             {
//                 queue.pop();
//                 job->state = Job::State::RUNNING;
//                 job->last_attempt = now;
//                 return job;
//             }
//             else
//                 // wait until earliest job is ready
//                 cv.wait_until(lock, job->ready_at, [this]
//                               { return !running; });
//         }
//         else cv.wait(lock, [this] {return !running;});
//     }

//     return nullptr;
// }

std::shared_ptr<Job> JobManager::acquire_job()
{
    std::unique_lock<std::mutex> lock(mtx);
    while (running)
    {
        if (!queue.empty())
        {
            auto job = queue.top();
            auto now = std::chrono::system_clock::now();

            if (job->is_ready())
            {
                queue.pop();
                job->state = Job::State::RUNNING;
                job->last_attempt = now;
                return job;
            }

            else
                // wait until earliest job is ready
                cv.wait_until(lock, job->ready_at, [this]
                              { return !running; });
        }
        else
        {
            if (!listen) break;

            cv.wait(lock, [this]
                    {return !running || !queue.empty();});
        }
    }
    return nullptr;
}

void JobManager::mark_success(const std::shared_ptr<Job> &job)
{
    std::lock_guard<std::mutex> lock(mtx);
    job->state = Job::State::DONE;
}

void JobManager::mark_failure(const std::shared_ptr<Job> &job)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (job->attempts >= maxRetries)
    {
        job->state = Job::State::FAILED;
        return;
    }

    job->state = Job::State::WAITING;
    job->ready_at = std::chrono::system_clock::now() + random_delay(job->id) + std::chrono::seconds(30);

    queue.push(job);
    cv.notify_one();
}

bool JobManager::empty() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
}
