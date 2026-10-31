#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace mxh::client {

// Production budgets. A failed connect is retried inside the 3s window.
// An ack that never arrives fails after 10s. Neither path resumes a
// dropped map session; the player logs in again.
inline constexpr std::chrono::milliseconds kLoginConnectTimeout{3000};
inline constexpr std::chrono::milliseconds kAgentConnectTimeout{3000};
inline constexpr std::chrono::seconds kLoginAckTimeout{10};
inline constexpr std::chrono::seconds kCharSelectAckTimeout{10};
inline constexpr std::chrono::seconds kCharMakeAckTimeout{10};
inline constexpr std::uint64_t kGameInAckTimeoutMs = 10000;

inline constexpr char kLoginAckTimeoutReason[] =
    "LoginAck timeout (no response from LoginServer)";
inline constexpr char kCharacterListAckTimeoutReason[] =
    "CharacterListAck timeout (no response from AgentServer)";
inline constexpr char kCharacterSelectAckTimeoutReason[] =
    "CharacterSelectAck timeout (no response from AgentServer)";
inline constexpr char kCharacterMakeAckTimeoutReason[] =
    "CharacterMakeAck timeout (no response from AgentServer)";
inline constexpr char kGameInAckTimeoutReason[] =
    "GameInAck timeout (no response from MapServer)";

// cMsgBox id passed to showMessage when the live session drops.
// GameStateId::Title is 3; CInGameState.cpp static_asserts that.
inline constexpr std::int32_t kDisconnectMessageId = 9106;
inline constexpr int kReloginStateId = 3;

inline bool ack_wait_expired(
    bool failed,
    bool finished,
    std::chrono::steady_clock::time_point deadline,
    std::chrono::steady_clock::time_point now) noexcept {
    return !failed && !finished &&
           deadline != std::chrono::steady_clock::time_point{} &&
           now >= deadline;
}

inline bool game_in_ack_expired(bool syn_sent,
                                std::uint64_t pending_since_ms,
                                std::uint64_t now_ms,
                                std::uint64_t budget_ms) noexcept {
    return syn_sent && pending_since_ms != 0 &&
           now_ms - pending_since_ms >= budget_ms;
}

// Process() calls send_gamein_syn while this is true. A failed send leaves
// the syn unlatched, so the next tick tries again. A success latches it.
inline bool should_resend_game_in_syn(bool connected, bool already_sent) noexcept {
    return connected && !already_sent;
}

inline bool latch_game_in_syn(bool send_ok) noexcept { return send_ok; }

struct GameInSynTick {
    bool attempted = false;
    bool sent = false;
};

inline GameInSynTick tick_game_in_syn(bool connected, bool already_sent,
                                      bool send_ok) noexcept {
    GameInSynTick tick;
    tick.sent = already_sent;
    if (!should_resend_game_in_syn(connected, already_sent)) return tick;
    tick.attempted = true;
    tick.sent = latch_game_in_syn(send_ok);
    return tick;
}

// True only for a live in-game session. The caller then shows message 9106
// and requests Title. Releasing, already-failed, or not-yet-in-game drops
// do not start another login by themselves.
inline bool in_game_disconnect_returns_to_title(bool releasing, bool in_game,
                                                 bool already_failed) noexcept {
    return !releasing && in_game && !already_failed;
}

struct DisconnectNotice {
    bool notify = false;
    std::int32_t message_id = 0;
    int next_state = -1;
};

inline DisconnectNotice in_game_disconnect_notice(bool releasing, bool in_game,
                                                   bool already_failed) noexcept {
    DisconnectNotice notice;
    if (!in_game_disconnect_returns_to_title(releasing, in_game, already_failed)) {
        return notice;
    }
    notice.notify = true;
    notice.message_id = kDisconnectMessageId;
    notice.next_state = kReloginStateId;
    return notice;
}

// A Start() that is already in flight is ignored. Release() clears the
// flag so the next Start() opens a new login instead of resuming.
inline bool ignore_duplicate_login_start(bool started) noexcept {
    return started;
}

inline void zero_secret(std::string& value) noexcept {
    volatile char* bytes = value.empty() ? nullptr : value.data();
    for (std::size_t i = 0; bytes && i < value.size(); ++i) bytes[i] = '\0';
}

inline void wipe_secret(std::string& value) noexcept {
    zero_secret(value);
    value.clear();
}

}  // namespace mxh::client
