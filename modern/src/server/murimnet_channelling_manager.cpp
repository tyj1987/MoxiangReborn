// murimnet_channelling_manager.cpp

#include "mxh/server/murimnet_channelling_manager.hpp"
#include <algorithm>
#include <cstring>

namespace mxh::server {

std::uint32_t MurimNetChannellingManager::create_channel(std::uint32_t creator_id) noexcept {
    if (creator_id == 0) return 0;
    if (channels_.size() >= MXH_MN_MAX_CHANNELS) return 0;
    Channel c{};
    c.id = next_ch_id_++;
    c.players.push_back(creator_id);
    c.state = MnChannelState::Active;
    channels_.push_back(c);
    return c.id;
}

bool MurimNetChannellingManager::destroy_channel(std::uint32_t channel_id) noexcept {
    auto it = std::find_if(channels_.begin(), channels_.end(),
                            [&](const Channel& c){ return c.id == channel_id; });
    if (it == channels_.end()) return false;
    channels_.erase(it);
    messages_.erase(std::remove_if(messages_.begin(), messages_.end(),
                                   [&](const MnChatMessage& message) {
                                       return message.channel_id == channel_id;
                                   }),
                    messages_.end());
    return true;
}

bool MurimNetChannellingManager::join(std::uint32_t channel_id, std::uint32_t player_id) noexcept {
    if (player_id == 0) return false;
    for (auto& c : channels_) {
        if (c.id == channel_id) {
            if (c.state != MnChannelState::Active) return false;
            if (c.players.size() >= MXH_MN_MAX_PLAYERS) return false;
            if (std::find(c.players.begin(), c.players.end(), player_id) != c.players.end())
                return false;
            c.players.push_back(player_id);
            return true;
        }
    }
    return false;
}

bool MurimNetChannellingManager::leave(std::uint32_t channel_id, std::uint32_t player_id) noexcept {
    for (auto& c : channels_) {
        if (c.id == channel_id) {
            auto it = std::find(c.players.begin(), c.players.end(), player_id);
            if (it == c.players.end()) return false;
            c.players.erase(it);
            if (c.players.empty()) c.state = MnChannelState::Closing;
            return true;
        }
    }
    return false;
}

bool MurimNetChannellingManager::send_message(const MnChatMessage& m) noexcept {
    if (m.channel_id == 0 || m.sender_id == 0) return false;
    auto exists = [&]{
        for (const auto& c : channels_) {
            if (c.id != m.channel_id || c.state != MnChannelState::Active) continue;
            return std::find(c.players.begin(), c.players.end(), m.sender_id) != c.players.end();
        }
        return false;
    }();
    if (!exists) return false;
    MnChatMessage msg = m;
    messages_.push_back(msg);
    return true;
}

std::vector<MnChatMessage> MurimNetChannellingManager::history(std::uint32_t channel_id,
                                                                 std::uint32_t max_count) const noexcept {
    std::vector<MnChatMessage> out;
    for (auto it = messages_.rbegin(); it != messages_.rend() && out.size() < max_count; ++it) {
        if (it->channel_id == channel_id) out.push_back(*it);
    }
    std::reverse(out.begin(), out.end());
    return out;
}

std::vector<std::uint32_t> MurimNetChannellingManager::members(std::uint32_t channel_id) const noexcept {
    for (const auto& c : channels_) if (c.id == channel_id) return c.players;
    return {};
}

MnChannelState MurimNetChannellingManager::state(std::uint32_t channel_id) const noexcept {
    for (const auto& c : channels_) if (c.id == channel_id) return c.state;
    return MnChannelState::Idle;
}

// -------- MurimNetChat --------

bool MurimNetChat::send(std::uint32_t sender_id, std::uint32_t map_num,
                          const std::string& body, std::uint32_t now_ms) noexcept {
    if (sender_id == 0 || body.empty()) return false;
    MnChatMessage m{};
    m.sender_id = sender_id;
    m.channel_id = map_num;   // local map channel uses map_num as routing key
    m.send_ms = now_ms;
    std::size_t k = std::min<std::size_t>(body.size(), 255);
    std::memcpy(m.text, body.data(), k);
    m.text[k] = '\0';
    messages_.push_back(m);
    return true;
}

std::vector<MnChatMessage> MurimNetChat::snapshot(std::uint32_t map_num) const noexcept {
    std::vector<MnChatMessage> out;
    for (const auto& m : messages_) if (m.channel_id == map_num) out.push_back(m);
    return out;
}

void MurimNetChat::clear_expired(std::uint32_t now_ms, std::uint32_t expire_ms) noexcept {
    std::vector<MnChatMessage> kept;
    kept.reserve(messages_.size());
    for (auto& m : messages_) {
        if (now_ms - m.send_ms < expire_ms) kept.push_back(m);
    }
    messages_.swap(kept);
}

}  // namespace mxh::server
