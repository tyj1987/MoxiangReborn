// distribute_server.hpp - 1:1 port of legacy [Server]Distribute/DistributeServer.h.
//
// The legacy DistributeServer is the login/character-select/map-routing hub.
// The modern port declares the same state machine + thread entry points
// while keeping the actual socket handling in mxh::net (Asio/IOCP) and
// protocol framing in mxh::proto.

#pragma once

#include "mxh/proto/protocol.hpp"
#include "mxh/net/net.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mxh::server {

inline constexpr std::uint16_t DEFAULT_DISTRIBUTE_PORT = 6001;

// Login state (legacy DistributeServer state machine).
enum class DistributeState : std::uint8_t {
    Stopped  = 0,
    Booting  = 1,
    Listening = 2,
    Running  = 3,
    Stopping = 4,
};

// Per-connection user record during login handshake.
struct LoginUserRecord final {
    std::uint32_t session_id   = 0;
    std::uint32_t user_id      = 0;
    std::uint8_t  auth_step    = 0;        // 1 = sent ID, 2 = waiting auth, 3 = char list
    std::uint8_t  reserved0    = 0;
    std::uint16_t reserved1    = 0;
    std::string   account_name;
    std::uint32_t last_active_ms = 0;
    std::vector<std::uint32_t> character_ids;  // owned character list
};

// DistributeServer main loop / state holder.
class DistributeServer final {
public:
    DistributeServer() = default;

    // Lifecycle: bind + listen + accept loop. bind() opens port, run()
    // blocks until stop(); stops cleanly on stop().
    bool bind(std::uint16_t port = DEFAULT_DISTRIBUTE_PORT) noexcept;
    void run() noexcept;
    void stop() noexcept;

    DistributeState state() const noexcept { return state_.load(); }

    // Stats / introspection.
    std::size_t session_count() const noexcept { return sessions_.size(); }
    std::vector<std::uint32_t> agent_endpoints() const noexcept;

private:
    std::atomic<DistributeState> state_{DistributeState::Stopped};
    std::uint16_t port_ = DEFAULT_DISTRIBUTE_PORT;
    std::unordered_map<std::uint32_t, LoginUserRecord> sessions_;
};

}  // namespace mxh::server
