// shout_manager.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/ShoutManager.h + ShoutManager.cpp (CShoutManager).
//
// The legacy manager owns a FIFO of shout messages. Character-scoped adds
// are limited to three queued messages, while the unconditional overload is
// used by trusted server-side producers. Every five seconds the first three
// messages are removed for one broadcast batch.
//
// Locked invariants (1:1 with legacy):
//   - MAX_SHOUT_LENGTH is 60, so the message buffer is 61 bytes including
//     its terminating byte.
//   - A character may have at most three messages in the whole FIFO. The
//     receive Count is the number of that character's messages after the
//     accepted add; Time is (queue_count / 3) * 5 seconds before append.
//   - The broadcast interval is 5000 milliseconds and a non-empty queue is
//     drained from the head in batches of at most three.
//   - Add operations copy the complete SHOUTBASE value into owned storage.
//
// Network fan-out is intentionally outside this data-side port: the legacy
// Process() sends the extracted SEND_SHOUTBASE to every online user.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>

namespace mxh::server {

inline constexpr std::size_t MAX_SHOUT_LENGTH = 60u;
inline constexpr std::size_t MAX_SHOUT_PER_CHARACTER = 3u;
inline constexpr std::uint32_t SHOUT_BROADCAST_INTERVAL_MS = 5000u;

struct ShoutBase {
    std::uint32_t CharacterIdx = 0;
    char Msg[MAX_SHOUT_LENGTH + 1] = {};
};

struct ShoutReceive {
    std::uint8_t Count = 0;
    std::uint16_t Time = 0;
    std::uint32_t CharacterIdx = 0;
};

struct ShoutBatch {
    std::uint8_t Count = 0;
    std::array<ShoutBase, MAX_SHOUT_PER_CHARACTER> ShoutMsg{};
};

struct ShoutManager {
    std::deque<ShoutBase> m_MsgList;
    std::uint32_t m_lastbrodtime = 0;
};

inline ShoutManager make_shout_manager() { return ShoutManager{}; }

inline void shout_manager_init(ShoutManager& m) {
    m.m_MsgList.clear();
    m.m_lastbrodtime = 0;
}

inline void shout_manager_release(ShoutManager& m) {
    m.m_MsgList.clear();
}

inline std::size_t shout_queue_size(const ShoutManager& m) {
    return m.m_MsgList.size();
}

inline bool add_shout_msg(ShoutManager& m,
                          const ShoutBase& base,
                          ShoutReceive& receive) {
    std::size_t msg_count = 0;
    for (const auto& queued : m.m_MsgList) {
        if (queued.CharacterIdx == base.CharacterIdx) ++msg_count;
        if (msg_count >= MAX_SHOUT_PER_CHARACTER) break;
    }
    if (msg_count >= MAX_SHOUT_PER_CHARACTER) {
        receive.Count = 0;
        return false;
    }
    receive.Count = static_cast<std::uint8_t>(msg_count + 1u);
    receive.Time = static_cast<std::uint16_t>(
        (m.m_MsgList.size() / MAX_SHOUT_PER_CHARACTER) * 5u);
    m.m_MsgList.push_back(base);
    return true;
}

inline void add_shout_msg(ShoutManager& m, const ShoutBase& base) {
    m.m_MsgList.push_back(base);
}

inline std::optional<ShoutBatch> take_shout_batch(ShoutManager& m,
                                                   std::uint32_t now_ms) {
    if ((now_ms - m.m_lastbrodtime) < SHOUT_BROADCAST_INTERVAL_MS) {
        return std::nullopt;
    }
    if (m.m_MsgList.empty()) return std::nullopt;

    ShoutBatch batch;
    while (!m.m_MsgList.empty() && batch.Count < MAX_SHOUT_PER_CHARACTER) {
        batch.ShoutMsg[batch.Count] = m.m_MsgList.front();
        m.m_MsgList.pop_front();
        ++batch.Count;
    }
    m.m_lastbrodtime = now_ms;
    return batch;
}

inline bool process(ShoutManager& m,
                    std::uint32_t now_ms,
                    ShoutBatch& batch_out) {
    auto batch = take_shout_batch(m, now_ms);
    if (!batch) return false;
    batch_out = *batch;
    return true;
}

} // namespace mxh::server