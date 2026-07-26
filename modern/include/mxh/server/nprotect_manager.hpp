// nprotect_manager.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/NProtectManager.h + NProtectManager.cpp.
//
// The missing GameGuard server SDK is represented by explicit vendor-call
// results and auth data. The AgentServer state machine, arithmetic, protocol
// values, disconnect decisions, and timers remain deterministic and testable.
//
// Locked invariants (1:1 with legacy):
//   - NProtect category is 69; QUERY=0, ANSWER=1, DISCONNECT=2,
//     USER_DISCONNECT=3, HACKTOOLUSER=4.
//   - SendAuthQuery rejects an outstanding query by sending DISCONNECT. It
//     calls DisconnectUser only when UserLevel >= eUSERLEVEL_GM (4).
//   - Query dwObjectID is value2*2 + index + value1. m_dwHUC is
//     index/2 + value1/2 + value2 + value3*2, using DWORD arithmetic.
//   - A valid first answer starts a second query. A valid second answer sets
//     state 3 and adds 120000 ms to dwLastNProtectCheck.
//   - Invalid answers disconnect and block. In steady state, a mismatched HUC
//     leaves m_bCSA set so the next query attempt disconnects the user.
//   - Initial authentication times out after 60000 ms; steady-state checks
//     are requested after 180000 ms; GameGuard updates run every 300000 ms.

#pragma once

#include <cstdint>

namespace mxh::server {

inline constexpr std::uint8_t NPROTECT_CATEGORY = 69u;
inline constexpr std::uint8_t NPROTECT_GM_LEVEL = 4u;
inline constexpr std::uint32_t NPROTECT_INITIAL_TIMEOUT_MS = 60000u;
inline constexpr std::uint32_t NPROTECT_STEADY_INTERVAL_MS = 180000u;
inline constexpr std::uint32_t NPROTECT_SECOND_ANSWER_OFFSET_MS = 120000u;
inline constexpr std::uint32_t NPROTECT_UPDATE_INTERVAL_MS = 300000u;
inline constexpr int NPLOG_DEBUG = 0x1;
inline constexpr int NPLOG_ERROR = 0x2;

enum class NProtectProtocol : std::uint8_t {
    Query = 0,
    Answer = 1,
    Disconnect = 2,
    UserDisconnect = 3,
    HackToolUser = 4,
};

enum class NProtectActionKind : std::uint8_t {
    None = 0,
    SendQuery = 1,
    SendDisconnect = 2,
    DisconnectAndBlock = 3,
};

struct NProtectAuthData {
    std::uint32_t dwIndex = 0;
    std::uint32_t dwValue1 = 0;
    std::uint32_t dwValue2 = 0;
    std::uint32_t dwValue3 = 0;
};

struct NProtectPacket {
    std::uint8_t Category = NPROTECT_CATEGORY;
    NProtectProtocol Protocol = NProtectProtocol::Query;
    std::uint32_t dwObjectID = 0;
    std::uint32_t dwData1 = 0;
    std::uint32_t dwData2 = 0;
    std::uint32_t dwData3 = 0;
    std::uint32_t dwData4 = 0;
};

struct NProtectAction {
    NProtectActionKind Kind = NProtectActionKind::None;
    NProtectPacket Packet{};
    bool ShouldDisconnectUser = false;
    bool ShouldBlock = false;
    std::uint32_t BlockType = 0;
};

struct NProtectUserState {
    std::uint32_t dwConnectionIndex = 0;
    std::uint32_t dwUserID = 0;
    std::uint32_t dwCharacterID = 0;
    std::uint8_t UserLevel = 0;
    NProtectAuthData m_AuthQuery{};
    NProtectAuthData m_AuthAnswer{};
    bool m_bCSA = false;
    int m_nCSAInit = 0;
    std::uint32_t dwLastNProtectCheck = 0;
    std::uint32_t m_dwHUC = 0;
    bool bHackToolUser = false;
};

struct NProtectManager {
    std::uint16_t m_MapNum = 0;
    bool m_Initialized = false;
    bool m_InitialUpdateRequested = false;
    bool m_UpdateClockStarted = false;
    std::uint32_t m_dwUpdateCheckTime = 0;
};

inline NProtectManager make_nprotect_manager() {
    return NProtectManager{};
}

inline bool nprotect_init(NProtectManager& manager,
                          std::uint16_t map_num,
                          bool vendor_init_succeeded) {
    manager.m_MapNum = map_num;
    manager.m_Initialized = vendor_init_succeeded;
    manager.m_InitialUpdateRequested = vendor_init_succeeded;
    return vendor_init_succeeded;
}

inline void nprotect_release(NProtectManager& manager) {
    manager.m_Initialized = false;
}

inline bool nprotect_update(NProtectManager& manager, std::uint32_t now_ms) {
    if (!manager.m_UpdateClockStarted) {
        manager.m_UpdateClockStarted = true;
        manager.m_dwUpdateCheckTime = now_ms;
        return false;
    }
    if ((now_ms - manager.m_dwUpdateCheckTime) < NPROTECT_UPDATE_INTERVAL_MS) {
        return false;
    }
    manager.m_dwUpdateCheckTime = now_ms;
    return true;
}

inline bool nprotect_should_log(int mode) {
    return (mode & (NPLOG_DEBUG | NPLOG_ERROR)) != 0;
}

inline NProtectAction nprotect_none() {
    return NProtectAction{};
}

inline NProtectAction nprotect_disconnect(bool disconnect_user) {
    NProtectAction action;
    action.Kind = NProtectActionKind::SendDisconnect;
    action.Packet.Protocol = NProtectProtocol::Disconnect;
    action.ShouldDisconnectUser = disconnect_user;
    return action;
}

inline NProtectAction send_auth_query(NProtectUserState& user,
                                       const NProtectAuthData& query,
                                       bool get_query_succeeded,
                                       std::uint32_t now_ms) {
    if (user.m_bCSA) {
        return nprotect_disconnect(user.UserLevel >= NPROTECT_GM_LEVEL);
    }
    if (!get_query_succeeded) return nprotect_none();

    user.m_AuthQuery = query;
    user.m_dwHUC = query.dwIndex / 2u + query.dwValue1 / 2u
        + query.dwValue2 + query.dwValue3 * 2u;
    user.m_bCSA = true;
    user.dwLastNProtectCheck = now_ms;

    NProtectAction action;
    action.Kind = NProtectActionKind::SendQuery;
    action.Packet.Protocol = NProtectProtocol::Query;
    action.Packet.dwData1 = query.dwIndex;
    action.Packet.dwData2 = query.dwValue1;
    action.Packet.dwData3 = query.dwValue2;
    action.Packet.dwData4 = query.dwValue3;
    action.Packet.dwObjectID = query.dwValue2 * 2u
        + query.dwIndex + query.dwValue1;
    return action;
}

inline NProtectAction parse_nprotect_answer(
    NProtectUserState* user,
    const NProtectPacket& answer,
    std::uint32_t check_result,
    const NProtectAuthData& next_query = {},
    bool get_next_query_succeeded = true,
    std::uint32_t now_ms = 0) {
    if (user == nullptr) return nprotect_none();

    user->m_AuthAnswer.dwIndex = answer.dwData1;
    user->m_AuthAnswer.dwValue1 = answer.dwData2;
    user->m_AuthAnswer.dwValue2 = answer.dwData3;
    user->m_AuthAnswer.dwValue3 = answer.dwData4;

    if (check_result != 0u) {
        NProtectAction action;
        action.Kind = NProtectActionKind::DisconnectAndBlock;
        action.Packet.Protocol = NProtectProtocol::Disconnect;
        action.ShouldDisconnectUser = true;
        action.ShouldBlock = true;
        action.BlockType = check_result;
        return action;
    }
    user->m_bCSA = false;
    if (user->m_nCSAInit == 3 && user->m_dwHUC != answer.dwObjectID) {
        user->m_bCSA = true;
    }

    if (user->m_nCSAInit == 1) {
        user->m_nCSAInit = 2;
        return send_auth_query(*user, next_query,
                               get_next_query_succeeded, now_ms);
    }
    if (user->m_nCSAInit == 2) {
        user->m_nCSAInit = 3;
        user->dwLastNProtectCheck += NPROTECT_SECOND_ANSWER_OFFSET_MS;
    }
    return nprotect_none();
}

inline void mark_hack_tool_user(NProtectUserState* user) {
    if (user != nullptr) user->bHackToolUser = true;
}

inline NProtectAction nprotect_check_tick(
    NProtectUserState& user,
    std::uint32_t now_ms,
    const NProtectAuthData& next_query = {},
    bool get_query_succeeded = true) {
    if (user.m_nCSAInit == 1 || user.m_nCSAInit == 2) {
        if ((now_ms - user.dwLastNProtectCheck) >= NPROTECT_INITIAL_TIMEOUT_MS) {
            user.m_nCSAInit = 4;
            return nprotect_disconnect(user.UserLevel >= NPROTECT_GM_LEVEL);
        }
        return nprotect_none();
    }

    if (user.m_nCSAInit == 3
        && (now_ms - user.dwLastNProtectCheck) >= NPROTECT_STEADY_INTERVAL_MS) {
        return send_auth_query(user, next_query, get_query_succeeded, now_ms);
    }
    return nprotect_none();
}

} // namespace mxh::server
