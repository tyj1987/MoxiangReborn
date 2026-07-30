// mn_server.hpp - 1:1 port of legacy [Server]MurimNet/MNServer.h.
//
// The legacy MNServer is a separate process hosting PvP rooms (channels).
// Modern port keeps the lifecycle and channel routing identical, with
// the actual networking owned by mxh::net.

#pragma once

#include "mxh/server/murimnet_channelling_manager.hpp"

#include <atomic>
#include <cstdint>
#include <unordered_map>

namespace mxh::server {

inline constexpr std::uint16_t MXH_MN_DEFAULT_PORT = 12001;

enum class MnServerState : std::uint8_t {
    Stopped  = 0,
    Booting  = 1,
    Listening = 2,
    Running  = 3,
};

// Per-room state (legacy MNPlayerInfo).
struct MnPlayerInfo final {
    std::uint32_t player_id = 0;
    std::uint32_t room_id   = 0;     // legacy ChannelIdx
    std::uint32_t last_seen_ms = 0;
    std::uint8_t  fighting  = 0;
    std::uint8_t  reserved0 = 0;
    std::uint16_t reserved1 = 0;
};

class MNServer final {
public:
    MNServer() = default;

    bool bind(std::uint16_t port = MXH_MN_DEFAULT_PORT) noexcept;
    void stop() noexcept;

    MnServerState state() const noexcept { return state_.load(); }

    // Active players / channel count.
    std::size_t session_count() const noexcept { return sessions_.size(); }
    std::size_t channel_count() const noexcept { return channels_.channel_count(); }

    // Convenience accessors to the underlying channel manager.
    MurimNetChannellingManager& channels() noexcept { return channels_; }

private:
    std::atomic<MnServerState> state_{MnServerState::Stopped};
    std::uint16_t port_ = MXH_MN_DEFAULT_PORT;
    MurimNetChannellingManager channels_;
    std::unordered_map<std::uint32_t, MnPlayerInfo> sessions_;
};

}  // namespace mxh::server
