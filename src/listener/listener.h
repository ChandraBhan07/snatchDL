#pragma once

#include <string>
#include <atomic>

template <typename T>
class ThreadSafeQueue;

class Listener
{
private:
    ThreadSafeQueue<std::string> &queue;

public:
    void run();
    Listener(ThreadSafeQueue<std::string> &queue) : queue(queue) {};
    void stop();
};