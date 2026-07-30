// murimnet_channelling_manager.hpp + chat
// 1:1 port of legacy MurimNet channel + chat manager.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::server {

// Channel states (legacy MNChannel::eChannelState).
enum class MnChannelState : std::uint8_t {
    Idle    = 0,
    Active  = 1,
    Closing = 2,
};

inline constexpr std::uint8_t MXH_MN_MAX_PLAYERS = 8;
inline constexpr std::uint16_t MXH_MN_MAX_CHANNELS = 200;

// Chat message (legacy MNChatMessage).
struct MnChatMessage final {
    std::uint32_t sender_id   = 0;
    std::uint32_t channel_id  = 0;
    std::uint32_t send_ms     = 0;
    std::uint8_t  kind        = 0;   // chat kind (party/guild/local/whisper)
    std::uint8_t  reserved0   = 0;
    std::uint16_t reserved1   = 0;
    char          text[256]   = {};
};

// Channelling manager: per-server list of chat channels.
class MurimNetChannellingManager final {
public:
    std::uint32_t create_channel(std::uint32_t creator_id) noexcept;
    bool destroy_channel(std::uint32_t channel_id) noexcept;
    bool join(std::uint32_t channel_id, std::uint32_t player_id) noexcept;
    bool leave(std::uint32_t channel_id, std::uint32_t player_id) noexcept;
    bool send_message(const MnChatMessage& m) noexcept;
    std::size_t channel_count() const noexcept { return channels_.size(); }
    std::vector<MnChatMessage> history(std::uint32_t channel_id,
                                         std::uint32_t max_count = 32) const noexcept;
    std::vector<std::uint32_t> members(std::uint32_t channel_id) const noexcept;
    MnChannelState state(std::uint32_t channel_id) const noexcept;

private:
    struct Channel {
        std::uint32_t id = 0;
        std::vector<std::uint32_t> players;
        MnChannelState state = MnChannelState::Idle;
    };
    std::vector<Channel> channels_;
    std::vector<MnChatMessage> messages_;
    std::uint32_t next_ch_id_  = 1;
    std::uint32_t next_msg_id_ = 1;
};

// MurimNetChat: simpler per-map local chat (legacy MNChatInfo).
class MurimNetChat final {
public:
    bool send(std::uint32_t sender_id, std::uint32_t map_num,
               const std::string& body, std::uint32_t now_ms) noexcept;
    std::vector<MnChatMessage> snapshot(std::uint32_t map_num) const noexcept;
    std::size_t size() const noexcept { return messages_.size(); }
    void clear_expired(std::uint32_t now_ms, std::uint32_t expire_ms = 60000) noexcept;
private:
    std::vector<MnChatMessage> messages_;
};

}  // namespace mxh::server
