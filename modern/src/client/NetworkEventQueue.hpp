#pragma once

#include "mxh/net/net.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace mxh::client {

enum class ClientRuntimeEventKind {
    Connected,
    Message,
    Disconnected,
    Error,
};

struct ClientRuntimeEvent {
    ClientRuntimeEventKind kind = ClientRuntimeEventKind::Error;
    mxh::net::ConnectionId connection{};
    mxh::net::Message message;
    mxh::net::NetError network_error = mxh::net::NetError::Ok;
    std::string detail;
};

// Bounded multi-producer/single-consumer queue. Network callbacks only copy
// immutable events into this queue; game state and UI mutation stays on the
// main thread when drain() is called from CGameState::Process().
class NetworkEventQueue {
public:
    explicit NetworkEventQueue(std::size_t capacity = 4096) noexcept;

    [[nodiscard]] bool push(ClientRuntimeEvent event);
    [[nodiscard]] std::vector<ClientRuntimeEvent> drain();
    // Predicate runs under the queue lock: it must not block or access this queue.
    // A rejected head remains in place, preserving FIFO and the capacity bound.
    [[nodiscard]] bool try_pop(ClientRuntimeEvent& event,
        const std::function<bool(const ClientRuntimeEvent&)>& ready);
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::uint64_t dropped_count() const;
    void clear();

private:
    const std::size_t m_capacity;
    mutable std::mutex m_mutex;
    std::deque<ClientRuntimeEvent> m_events;
    std::uint64_t m_dropped = 0;
};

}  // namespace mxh::client
