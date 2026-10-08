#pragma once

#include <cstdlib>

namespace mxh::net {

// Packet payloads can include authentication material. Keep routine packet
// tracing off unless a diagnostic process explicitly opts in.
inline bool packet_trace_enabled() noexcept {
    static const bool enabled = [] {
        const char* value = std::getenv("MXH_PACKET_TRACE");
        return value != nullptr && value[0] == '1' && value[1] == '\0';
    }();
    return enabled;
}

}  // namespace mxh::net
