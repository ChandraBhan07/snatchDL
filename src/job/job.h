#pragma once

#include <chrono>
#include <string>

class Job
{
public:
    enum class State
    {
        RUNNING,
        WAITING,
        SUCCEEDED,
        FAILED
    };

    unsigned int attempts = 1;
    State state = State::WAITING;

    std::string id;
    std::string url;
    std::string serviceKey;

    std::chrono::steady_clock::time_point readyAt;
    std::chrono::steady_clock::time_point lastAattempt;

    explicit Job(std::string id, std::string key, std::string url)
        : id(id), serviceKey(key), url(url) {}

    bool isReady() const
    {
        return state == State::WAITING &&
               std::chrono::steady_clock::now() >= readyAt;
    }
};
