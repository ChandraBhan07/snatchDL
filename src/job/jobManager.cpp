#include "jobManager.h"
#include "../runtime/threadSafeQueue.h"
#include "../runtime/spread.h"
#include "job.h"
#include "jobHistory.h"

#include <ctime>
#include <iomanip>
#include <iostream>

void JobManager::run()
{
    int maxAttempts = 2;

    while (incomingQueue.wait())
    {
        auto job = incomingQueue.tryPop();

        if (!job)
            continue;

        if (job->id.empty())
            continue;

        if (job->state == Job::State::FAILED)
        {
            if (job->attempts > maxAttempts)
            {
                // failed
                jobHistory.recordFailure(job->serviceKey, job->id);
                continue;
            }
        }

        if (job->state == Job::State::SUCCEEDED)
        {
            jobHistory.recordSuccess(job->serviceKey, job->id);
            continue;
        }

        // schedule
        static thread_local std::mt19937 rng(std::random_device{}());
        int delay = Spread::random(67, 393, rng);

        std::chrono::seconds wait = std::chrono::seconds(delay) + startDelay;

        job->readyAt = std::chrono::steady_clock::now() + wait;
        job->state = Job::State::WAITING;

        auto scheduledTime = std::chrono::system_clock::now() + wait;

        std::time_t time = std::chrono::system_clock::to_time_t(scheduledTime);

        std::cout << "Job scheduled with id: " << job->id
                  << " at "
                  << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
                  << '\n';

        outgoingQueue.enqueue(std::move(*job));
    }
}