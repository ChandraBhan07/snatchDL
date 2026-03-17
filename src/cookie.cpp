#include "cookie.h"

bool Cookie::ready() const {
    if (state != State::ACTIVE)
        return false;

    return std::chrono::system_clock::now() >= next_allowed;
}
