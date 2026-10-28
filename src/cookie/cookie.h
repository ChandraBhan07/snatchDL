#pragma once

#include <string>
#include <filesystem>
#include <chrono>

class Cookie
{
public:
    enum class State
    {
        ACTIVE,
        RESTING,
        RETIRED
    };

    std::filesystem::path path;

    Cookie(std::filesystem::path p)
        : path(std::move(p)), state(State::ACTIVE), nextAllowed(std::chrono::steady_clock::now()) {}

    // state
    State state;

    // timing
    std::chrono::steady_clock::time_point nextAllowed;
    std::chrono::steady_clock::time_point lastUsed{};

    // health (slow to recover, fast to degrade)
    int trust = 100;

    bool ready() const
    {
        if (state != State::ACTIVE)
            return false;

        return std::chrono::steady_clock::now() >= nextAllowed;
    }
};
