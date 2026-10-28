#pragma once

#include <string>
#include <functional>
#include <chrono>

template <typename T>
class ThreadSafeQueue;

class Job;

class JobHistory;

class JobManager
{

    ThreadSafeQueue<Job> &incomingQueue;
    ThreadSafeQueue<Job> &outgoingQueue;
    JobHistory &jobHistory;
    std::chrono::seconds &startDelay;

public:
    void run();

    JobManager(
        ThreadSafeQueue<Job> &incoming,
        ThreadSafeQueue<Job> &outgoing,
        JobHistory &history, std::chrono::seconds &startDelay)
        : incomingQueue(incoming), outgoingQueue(outgoing),
          jobHistory(history), startDelay(startDelay) {};
};