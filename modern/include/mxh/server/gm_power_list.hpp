// gm_power_list.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/GMPowerList.h + GMPowerList.cpp (CGMPowerList).
//
// GMPowerList stores the GM privilege assigned to each live connection.
// Network transmission is represented as a deterministic message value; the
// legacy g_Network.Send2User integration remains an AgentServer glue concern.
//
// Locked invariants (1:1 with legacy):
//   - eGM_POWER values are MASTER=0, MONITOR=1, PATROLLER=2, AUDITOR=3,
//     EVENTER=4, MAX=5.
//   - AddGMList appends a new record and copies at most MAX_NAME_LENGTH
//     characters plus a terminating byte into szGMID.
//   - When MonitorRevolution.gmp is present, a MONITOR login is elevated to
//     MASTER exactly as the legacy temporary monitor-cheat hook does.
//   - RemoveGMList and lookup operations use the first matching connection,
//     preserving the legacy FIFO-list behavior.
//   - Missing connections return -1 from GetGMPower and nullptr from
//     GetGMInfo. Release clears records but does not reset the two event flags.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <list>
#include <string_view>

namespace mxh::server {

inline constexpr std::size_t GM_MAX_NAME_LENGTH = 20u;
inline constexpr std::uint8_t GM_CHEAT_CATEGORY = 11u;
inline constexpr std::uint8_t GM_LOGIN_ACK_PROTOCOL = 81u;
inline constexpr std::uint8_t GM_LOGIN_NACK_PROTOCOL = 82u;

enum class GmPower : int {
    Master = 0,
    Monitor = 1,
    Patroller = 2,
    Auditor = 3,
    Eventer = 4,
    Max = 5,
};

struct GmInfo {
    std::uint32_t dwConnectionIndex = 0;
    std::uint32_t dwGMIndex = 0;
    char szGMID[GM_MAX_NAME_LENGTH + 1] = {};
    int nPower = 0;
};

struct GmLoginMessage {
    std::uint8_t Category = GM_CHEAT_CATEGORY;
    std::uint8_t Protocol = GM_LOGIN_NACK_PROTOCOL;
    std::uint32_t dwObjectID = 0;
    std::uint32_t dwData = 0;
    bool HasData = false;
};

struct GmPowerList {
    std::list<GmInfo> m_listGPL;
    bool m_bMonitorCheat = false;
    bool m_bEvent1Start = false;
};

inline GmPowerList make_gm_power_list() {
    return GmPowerList{};
}

inline void gm_power_list_init(GmPowerList& m, bool monitor_file_exists) {
    if (monitor_file_exists) m.m_bMonitorCheat = true;
}

inline void gm_power_list_init(GmPowerList& m) {
    if (std::filesystem::exists("./MonitorRevolution.gmp")) {
        m.m_bMonitorCheat = true;
    }
}

inline void gm_power_list_release(GmPowerList& m) {
    m.m_listGPL.clear();
}

inline std::size_t gm_info_count(const GmPowerList& m) {
    return m.m_listGPL.size();
}

inline void add_gm_list(GmPowerList& m,
                        std::uint32_t connection_index,
                        int power,
                        std::uint32_t gm_index,
                        std::string_view gm_id) {
    GmInfo info;
    info.dwConnectionIndex = connection_index;
    info.nPower = power;
    info.dwGMIndex = gm_index;
    const auto copy_length = gm_id.size() < GM_MAX_NAME_LENGTH
        ? gm_id.size()
        : GM_MAX_NAME_LENGTH;
    std::memcpy(info.szGMID, gm_id.data(), copy_length);
    info.szGMID[copy_length] = 0;

    if (power == static_cast<int>(GmPower::Monitor) && m.m_bMonitorCheat) {
        info.nPower = static_cast<int>(GmPower::Master);
    }
    m.m_listGPL.push_back(info);
}

inline void add_gm_list(GmPowerList& m,
                        std::uint32_t connection_index,
                        GmPower power,
                        std::uint32_t gm_index,
                        std::string_view gm_id) {
    add_gm_list(m, connection_index, static_cast<int>(power), gm_index, gm_id);
}

inline bool remove_gm_list(GmPowerList& m, std::uint32_t connection_index) {
    for (auto it = m.m_listGPL.begin(); it != m.m_listGPL.end(); ++it) {
        if (it->dwConnectionIndex == connection_index) {
            m.m_listGPL.erase(it);
            return true;
        }
    }
    return false;
}

inline int get_gm_power(const GmPowerList& m, std::uint32_t connection_index) {
    for (const auto& info : m.m_listGPL) {
        if (info.dwConnectionIndex == connection_index) return info.nPower;
    }
    return -1;
}

inline GmInfo* get_gm_info(GmPowerList& m, std::uint32_t connection_index) {
    for (auto& info : m.m_listGPL) {
        if (info.dwConnectionIndex == connection_index) return &info;
    }
    return nullptr;
}

inline const GmInfo* get_gm_info(const GmPowerList& m,
                                 std::uint32_t connection_index) {
    for (const auto& info : m.m_listGPL) {
        if (info.dwConnectionIndex == connection_index) return &info;
    }
    return nullptr;
}

inline void set_event_cheat(GmPowerList& m, bool started) {
    m.m_bEvent1Start = started;
}

inline bool is_event_started(const GmPowerList& m) {
    return m.m_bEvent1Start;
}

inline GmLoginMessage make_gm_login_success(std::uint32_t object_id, int power) {
    GmLoginMessage message;
    message.Protocol = GM_LOGIN_ACK_PROTOCOL;
    message.dwObjectID = object_id;
    message.dwData = static_cast<std::uint32_t>(power);
    message.HasData = true;
    return message;
}

inline GmLoginMessage make_gm_login_fail(std::uint32_t object_id) {
    GmLoginMessage message;
    message.Protocol = GM_LOGIN_NACK_PROTOCOL;
    message.dwObjectID = object_id;
    return message;
}

} // namespace mxh::server