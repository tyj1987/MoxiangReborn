// server_list.hpp - 1:1 port of legacy [Server]Distribute/ServerList.h.
//
// Legacy ServerList holds a flat vector of agent+map endpoints
// indexed by channel number. Each entry carries the IP+port pair
// the client uses to connect after Distribute tells them which
// agent/map to hop to.
//
// Wire format: SEND_SERVERLIST (legacy) reads an array of
// SERVERLIST_MSG ready to send to the client. Modern port keeps
// the same shape.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace mxh::server {

// Legacy SERVERLIST_INFO wire struct (~24 bytes).
struct ServerListEntry final {
    std::uint16_t channel_num = 0;     // 0..MAX_CHANNEL
    std::uint16_t map_num     = 0;
    std::uint8_t  kind        = 0;     // 0 = agent, 1 = map
    std::uint8_t  reserved0   = 0;
    std::uint16_t reserved1   = 0;
    std::array<char, 16> ip{};          // legacy ip_str[16]
    std::uint16_t port       = 0;
};

class ServerList final {
public:
    // Register endpoints. The (channel, map) is the routing key.
    bool add(const ServerListEntry& e) noexcept;
    // Find the lowest-load endpoint for kind+map.
    const ServerListEntry* find_for_map(std::uint16_t map_num,
                                          std::uint8_t kind) const noexcept;
    // Iterate all entries (legacy writes them as a single SEND_SERVERLIST).
    std::vector<ServerListEntry> snapshot() const noexcept { return entries_; }
    std::size_t size() const noexcept { return entries_.size(); }
    void clear() noexcept { entries_.clear(); }
private:
    std::vector<ServerListEntry> entries_;
};

}  // namespace mxh::server
