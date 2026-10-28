#pragma once

#include <string>

template <typename t>
class ThreadSafeQueue;

class InputFile;

class Job;
class JobBuilder;


class ListenerWorker
{
    ThreadSafeQueue<std::string> &listenerQueue;
    ThreadSafeQueue<Job> &incomingQueue;
    JobBuilder &jobBuilder;
    InputFile &inputFile;

public:
    ListenerWorker(ThreadSafeQueue<std::string> &listenQueue,
                   ThreadSafeQueue<Job> &inQueue,
                   JobBuilder &jobBuilder,
                   InputFile &inputFile) : listenerQueue(listenQueue),
                                           incomingQueue(inQueue),
                                           jobBuilder(jobBuilder),
                                           inputFile(inputFile) {};

    void run();
};