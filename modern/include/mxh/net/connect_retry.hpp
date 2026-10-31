#pragma once

#include <chrono>
#include <utility>

namespace mxh::net {

// Spacing between failed TCP connect attempts. TcpClient::connect uses this
// with steady_clock so a transient refusal does not abort the login budget.
inline constexpr std::chrono::milliseconds kConnectRetryInterval{50};

// Try `attempt` until it succeeds or `budget` has elapsed. A failed attempt
// sleeps one interval only when the clock is still inside the budget.
// `now` is sampled for the deadline and after every failure.
template <class Now, class Sleep, class Attempt>
bool retry_connect_until(std::chrono::milliseconds budget,
                         Now&& now,
                         Sleep&& sleep,
                         Attempt&& attempt) {
    const auto deadline = now() + budget;
    do {
        if (attempt()) return true;
        if (now() >= deadline) return false;
        sleep(kConnectRetryInterval);
    } while (true);
}

}  // namespace mxh::net
