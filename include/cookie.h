#pragma once
#include <string>
#include <filesystem>
#include <chrono>

class Cookie {
public:
    enum class State {
        ACTIVE,
        RESTING,
        RETIRED
    };

    std::string name;
    std::filesystem::path path;
    std::string source;

    Cookie(std::string n, std::filesystem::path p, std::string s)
        : name(std::move(n)), path(std::move(p)), source(std::move(s)) {}

    // state
    State state = State::ACTIVE;

    // timing
    std::chrono::system_clock::time_point next_allowed = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point last_used{};

    // health (slow to recover, fast to degrade)
    int trust = 100;

    bool ready() const;
};
