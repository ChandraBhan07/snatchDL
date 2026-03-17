#pragma once
#include <string>
#include <chrono>

class Job
{
public:
    enum class State
    {
        RUNNING,
        WAITING,
        DONE,
        FAILED
    };

    std::string id;
    std::string source;

    unsigned int attempts = 1;
    State state = State::WAITING;

    std::chrono::system_clock::time_point ready_at;
    std::chrono::system_clock::time_point last_attempt;

    Job(std::string id, std::string source) : id(std::move(id)), source(std::move(source)) {}

    bool is_ready() const
    {
        return state == State::WAITING &&
               std::chrono::system_clock::now() >= ready_at;
    }
};
