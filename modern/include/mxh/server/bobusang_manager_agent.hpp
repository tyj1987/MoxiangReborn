// bobusang_manager_agent.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/BobusangManager_Agent.h + BobusangManager_Agent.cpp.
//
// The legacy manager schedules one traveling merchant per channel and sends
// appearance/disappearance messages to the corresponding MapServer. Modern
// code keeps scheduling and message decisions pure; transport is represented
// by BobusangOutbound so the AgentServer network layer can attach later.
//
// Locked invariants (1:1 with legacy):
//   - BOBUSANG_POSNUM_MAX=4, DEALITEM_BIN_TABNUM=7,
//     BOBUSANG_CHECKTIME=60000.
//   - State values are NONE=-1, DISAPPEAR=0, APPEAR=2,
//     APPEAR_DELAYED=4, DISAPPEAR_DELAYED=8, TIME_DELAYED=16.
//   - A channel owns CUR and NEXT BOBUSANGINFO records. Appearance schedules
//     choose a map, a gap in minutes, a duration in minutes, a position index,
//     and a selling-list tab; all choices remain explicit for deterministic
//     tests and can be fed by legacy rand()/bin readers in production glue.
//   - Process runs only when now_ms is strictly greater than the last check
//     plus 60000, matching legacy's `gCurTime > check + interval` test.
//   - One minute before disappearance the legacy sends a notify; after the
//     disappearance time it sends the leave message and enters delayed state.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

inline constexpr std::size_t BOBUSANG_POSNUM_MAX = 4u;
inline constexpr std::size_t DEALITEM_BIN_TABNUM = 7u;
inline constexpr std::uint32_t BOBUSANG_CHECKTIME = 60000u;
inline constexpr std::uint8_t BOBUSANG_CATEGORY = 74u;
inline constexpr std::uint8_t BOBUSANG_INFO_AGENT_TO_MAP = 0u;
inline constexpr std::uint8_t BOBUSANG_DISAPPEAR_AGENT_TO_MAP = 1u;
inline constexpr std::uint8_t BOBUSANG_NOTIFY_FOR_DISAPPEARANCE = 12u;
inline constexpr std::uint8_t CHEAT_BOBUSANG_INFO_ACK = 149u;
inline constexpr std::uint8_t CHEAT_BOBUSANG_INFO_NACK = 150u;
inline constexpr std::uint8_t CHEAT_BOBUSANG_LEAVE_ACK = 152u;
inline constexpr std::uint8_t CHEAT_BOBUSANG_LEAVE_NACK = 153u;
inline constexpr std::uint8_t CHEAT_BOBUSANG_CHANGE_ACK = 155u;
inline constexpr std::uint8_t CHEAT_BOBUSANG_ONOFF_ACK = 158u;
inline constexpr std::uint8_t CHEAT_BOBUSANG_ONOFF_NACK = 159u;

enum BobusangInfoTime : int {
    eBBSIT_CUR = 0,
    eBBSIT_NEXT = 1,
    eBBSIT_KINDNUM = 2,
};

enum BobusangAppearedState : int {
    eBBSAS_NONE = -1,
    eBBSAS_DISAPPEAR = 0,
    eBBSAS_APPEAR = 2,
    eBBSAS_APPEAR_DELAYED = 4,
    eBBSAS_DISAPPEAR_DELAYED = 8,
    eBBSAS_TIME_DELAYED = 16,
};

struct BobusangPosPerMap {
    float ApprX = 0.0f;
    float ApprZ = 0.0f;
    float ApprDir = 0.0f;
};

struct BobusangMapInfo {
    std::uint32_t dataIdx = 0;
    std::uint32_t mapNum = 0;
    bool MapServerConnected = true;
    std::array<BobusangPosPerMap, BOBUSANG_POSNUM_MAX> Pos{};
};

struct BobusangInfo {
    std::uint32_t AppearanceMapNum = 0;
    std::uint32_t AppearanceChannel = 0;
    std::uint32_t AppearanceTime = 0;
    std::uint32_t DisappearanceTime = 0;
    std::uint32_t AppearancePosIdx = 0;
    std::uint32_t SellingListIndex = 0;
};

struct BobusangRandomChoice {
    std::size_t map_index = 0;
    std::uint32_t appearance_gap_minutes = 0;
    std::uint32_t duration_minutes = 0;
    std::uint32_t position_index = 0;
    std::uint32_t selling_list_index = 0;
};

struct BobusangOutbound {
    enum class Kind : std::uint8_t {
        InfoToMap,
        LeaveToMap,
        NotifyDisappearance,
    };

    Kind kind = Kind::InfoToMap;
    std::uint32_t map_num = 0;
    std::uint32_t channel = 0;
    std::uint32_t disappearance_time = 0;
    BobusangInfo info{};
};

struct BobusangDeveloperReply {
    std::uint8_t protocol = 0;
    std::uint32_t data = 0;
    int appearance_state = eBBSAS_NONE;
    std::array<BobusangInfo, 2> info{};
};

struct BobusangManagerAgent {
    bool m_bManager = false;
    bool m_bOnProcessing = true;
    std::uint32_t m_dwBobusangCheckTime = 0;
    std::uint32_t m_nChannelTotalNum = 0;
    std::uint32_t m_dwAppearTimeMin = 0;
    std::uint32_t m_dwAppearTimeMax = 0;
    std::uint32_t m_dwDurationTimeMin = 0;
    std::uint32_t m_dwDurationTimeMax = 0;
    std::vector<int> m_AppearedState;
    std::vector<BobusangInfo> m_BobusangInfo;
    std::vector<BobusangMapInfo> m_MapInfo;
};

struct BobusangChannelRecord {
    std::uint16_t map_num = 0;
    std::uint16_t kind = 0;
    std::uint32_t max_channel_num = 0;
};

inline std::uint32_t bobusang_pack_time(std::uint32_t year,
                                         std::uint32_t month,
                                         std::uint32_t day,
                                         std::uint32_t hour,
                                         std::uint32_t minute,
                                         std::uint32_t second) {
    return (year << 28u) | (month << 24u) | (day << 18u)
        | (hour << 12u) | (minute << 6u) | second;
}

inline std::uint32_t bobusang_time_year(std::uint32_t value) { return value >> 28u; }
inline std::uint32_t bobusang_time_month(std::uint32_t value) { return (value >> 24u) & 0x0fu; }
inline std::uint32_t bobusang_time_day(std::uint32_t value) { return (value >> 18u) & 0x3fu; }
inline std::uint32_t bobusang_time_hour(std::uint32_t value) { return (value >> 12u) & 0x3fu; }
inline std::uint32_t bobusang_time_minute(std::uint32_t value) { return (value >> 6u) & 0x3fu; }
inline std::uint32_t bobusang_time_second(std::uint32_t value) { return value & 0x3fu; }

inline std::uint64_t bobusang_time_to_minutes(std::uint32_t value) {
    return (static_cast<std::uint64_t>(bobusang_time_year(value)) * 360u * 24u * 60u)
        + (static_cast<std::uint64_t>(bobusang_time_month(value) - 1u) * 30u * 24u * 60u)
        + (static_cast<std::uint64_t>(bobusang_time_day(value) - 1u) * 24u * 60u)
        + (static_cast<std::uint64_t>(bobusang_time_hour(value)) * 60u)
        + bobusang_time_minute(value);
}
inline std::uint32_t bobusang_add_minutes(std::uint32_t value,
                                           std::uint32_t minutes) {
    auto year = bobusang_time_year(value);
    auto month = bobusang_time_month(value);
    auto day = bobusang_time_day(value);
    auto hour = bobusang_time_hour(value);
    auto minute = bobusang_time_minute(value);
    auto second = bobusang_time_second(value);
    minute += minutes;
    hour += minute / 60u;
    minute %= 60u;
    day += hour / 24u;
    hour %= 24u;
    while (day > 30u) {
        day -= 30u;
        ++month;
    }
    while (month > 12u) {
        month -= 12u;
        ++year;
    }
    return bobusang_pack_time(year, month, day, hour, minute, second);
}

inline BobusangManagerAgent make_bobusang_manager_agent() {
    return BobusangManagerAgent{};
}

inline void bobusang_manager_set_manager(BobusangManagerAgent& manager,
                                          bool is_manager) {
    manager.m_bManager = is_manager;
}

inline void bobusang_manager_start(BobusangManagerAgent& manager,
                                   std::uint32_t server_num) {
    manager.m_bManager = server_num == 0u;
}

inline bool load_bobusang_channels(
    BobusangManagerAgent& manager,
    const std::vector<BobusangChannelRecord>& records) {
    std::uint32_t max_channel = 0;
    for (const auto& record : records) {
        if (record.max_channel_num > max_channel) {
            max_channel = record.max_channel_num;
        }
    }
    manager.m_nChannelTotalNum = max_channel;
    manager.m_AppearedState.assign(max_channel, eBBSAS_NONE);
    manager.m_BobusangInfo.assign(max_channel * eBBSIT_KINDNUM, BobusangInfo{});
    return max_channel != 0u;
}

inline bool configure_bobusang_schedule(BobusangManagerAgent& manager,
                                         std::uint32_t appearance_min,
                                         std::uint32_t appearance_max,
                                         std::uint32_t duration_min,
                                         std::uint32_t duration_max,
                                         const std::vector<BobusangMapInfo>& maps) {
    manager.m_dwAppearTimeMin = appearance_min;
    manager.m_dwAppearTimeMax = appearance_max;
    manager.m_dwDurationTimeMin = duration_min;
    manager.m_dwDurationTimeMax = duration_max;
    manager.m_MapInfo = maps;
    return !maps.empty() && appearance_max >= appearance_min
        && duration_max >= duration_min;
}

inline void bobusang_manager_release(BobusangManagerAgent& manager) {
    manager.m_AppearedState.clear();
    manager.m_BobusangInfo.clear();
    manager.m_nChannelTotalNum = 0;
}

inline int bobusang_channel_state(const BobusangManagerAgent& manager,
                                  std::uint32_t channel) {
    if (channel >= manager.m_AppearedState.size()) return eBBSAS_NONE;
    return manager.m_AppearedState[channel];
}

inline const BobusangInfo* bobusang_info(const BobusangManagerAgent& manager,
                                         std::uint32_t channel,
                                         BobusangInfoTime which) {
    const auto index = static_cast<std::size_t>(channel) * eBBSIT_KINDNUM
        + static_cast<int>(which);
    if (index >= manager.m_BobusangInfo.size()) return nullptr;
    return &manager.m_BobusangInfo[index];
}

inline BobusangInfo* bobusang_info(BobusangManagerAgent& manager,
                                   std::uint32_t channel,
                                   BobusangInfoTime which) {
    const auto index = static_cast<std::size_t>(channel) * eBBSIT_KINDNUM
        + static_cast<int>(which);
    if (index >= manager.m_BobusangInfo.size()) return nullptr;
    return &manager.m_BobusangInfo[index];
}

inline bool set_bobusang_info(BobusangManagerAgent& manager,
                              std::uint32_t channel,
                              std::uint32_t now_time,
                              const BobusangRandomChoice& choice) {
    if (channel >= manager.m_nChannelTotalNum || manager.m_MapInfo.empty()) {
        if (channel < manager.m_AppearedState.size()) {
            manager.m_AppearedState[channel] = eBBSAS_NONE;
        }
        return false;
    }
    const auto map_index = choice.map_index % manager.m_MapInfo.size();
    const auto& map = manager.m_MapInfo[map_index];
    if (!map.MapServerConnected) {
        manager.m_AppearedState[channel] = eBBSAS_NONE;
        return false;
    }

    auto* next = bobusang_info(manager, channel, eBBSIT_NEXT);
    auto* current = bobusang_info(manager, channel, eBBSIT_CUR);
    if (next == nullptr || current == nullptr) return false;
    next->AppearanceChannel = channel;
    next->AppearanceMapNum = map.mapNum;
    const auto start = current->DisappearanceTime > now_time
        ? current->DisappearanceTime
        : now_time;
    next->AppearanceTime = bobusang_add_minutes(
        start, choice.appearance_gap_minutes);
    next->DisappearanceTime = bobusang_add_minutes(
        next->AppearanceTime, choice.duration_minutes);
    next->AppearancePosIdx = choice.position_index % BOBUSANG_POSNUM_MAX;
    next->SellingListIndex = choice.selling_list_index % DEALITEM_BIN_TABNUM;
    if (manager.m_AppearedState[channel] != eBBSAS_APPEAR) {
        manager.m_AppearedState[channel] = eBBSAS_DISAPPEAR;
    }
    return true;
}

inline void initialize_bobusang_info(BobusangManagerAgent& manager,
                                     std::uint32_t now_time,
                                     const BobusangRandomChoice& choice) {
    if (!manager.m_bManager) return;
    for (std::uint32_t channel = 0; channel < manager.m_nChannelTotalNum; ++channel) {
        set_bobusang_info(manager, channel, now_time, choice);
    }
}

inline bool set_channel_state(BobusangManagerAgent& manager,
                              std::uint32_t channel,
                              int state) {
    if (!manager.m_bManager || channel >= manager.m_AppearedState.size()) {
        return false;
    }
    manager.m_AppearedState[channel] = state;
    return true;
}

inline std::vector<BobusangOutbound> bobusang_process(
    BobusangManagerAgent& manager,
    std::uint32_t now_ms,
    std::uint32_t now_time,
    const BobusangRandomChoice& choice) {
    std::vector<BobusangOutbound> out;
    if (!manager.m_bManager || now_ms <= manager.m_dwBobusangCheckTime
        + BOBUSANG_CHECKTIME || manager.m_nChannelTotalNum == 0u) {
        return out;
    }

    for (std::uint32_t channel = 0; channel < manager.m_nChannelTotalNum; ++channel) {
        auto state = manager.m_AppearedState[channel];
        if (state == eBBSAS_APPEAR) {
            auto* current = bobusang_info(manager, channel, eBBSIT_CUR);
            if (current == nullptr) continue;
            const auto now_minutes = bobusang_time_to_minutes(now_time);
            const auto disappearance_minutes = bobusang_time_to_minutes(
                current->DisappearanceTime);
            if (now_minutes + 1u >= disappearance_minutes
                && now_time <= current->DisappearanceTime) {
                out.push_back({BobusangOutbound::Kind::NotifyDisappearance,
                               current->AppearanceMapNum, channel,
                               current->DisappearanceTime, *current});
            }
            if (now_time > current->DisappearanceTime) {
                out.push_back({BobusangOutbound::Kind::LeaveToMap,
                               current->AppearanceMapNum, channel,
                               current->DisappearanceTime, *current});
                manager.m_AppearedState[channel] = eBBSAS_DISAPPEAR_DELAYED;
            }
        } else if (state == eBBSAS_DISAPPEAR) {
            auto* next = bobusang_info(manager, channel, eBBSIT_NEXT);
            if (next == nullptr || now_time <= next->AppearanceTime) continue;
            if (!manager.m_bOnProcessing) return out;
            out.push_back({BobusangOutbound::Kind::InfoToMap,
                           next->AppearanceMapNum, channel,
                           next->DisappearanceTime, *next});
            *bobusang_info(manager, channel, eBBSIT_CUR) = *next;
            manager.m_AppearedState[channel] = eBBSAS_APPEAR_DELAYED;
        } else if (state == eBBSAS_APPEAR_DELAYED
                   || state == eBBSAS_DISAPPEAR_DELAYED) {
            auto* next = bobusang_info(manager, channel, eBBSIT_NEXT);
            if (next != nullptr && now_time > bobusang_add_minutes(
                    next->AppearanceTime, manager.m_dwAppearTimeMax)) {
                set_bobusang_info(manager, channel, now_time, choice);
            }
        } else if (state == eBBSAS_NONE) {
            set_bobusang_info(manager, channel, now_time, choice);
        }
    }
    manager.m_dwBobusangCheckTime = now_ms;
    return out;
}

inline bool set_bobusang_processing(BobusangManagerAgent& manager, bool value) {
    if (manager.m_bOnProcessing == value) return false;
    manager.m_bOnProcessing = value;
    return true;
}

inline BobusangDeveloperReply developer_bobusang_info(
    const BobusangManagerAgent& manager, std::uint32_t channel) {
    BobusangDeveloperReply reply;
    if (channel >= manager.m_nChannelTotalNum) {
        reply.protocol = CHEAT_BOBUSANG_INFO_NACK;
        return reply;
    }
    reply.protocol = CHEAT_BOBUSANG_INFO_ACK;
    reply.appearance_state = manager.m_AppearedState[channel];
    if (const auto* current = bobusang_info(manager, channel, eBBSIT_CUR)) {
        reply.info[0] = *current;
    }
    if (const auto* next = bobusang_info(manager, channel, eBBSIT_NEXT)) {
        reply.info[1] = *next;
    }
    return reply;
}

inline BobusangDeveloperReply developer_bobusang_leave(
    BobusangManagerAgent& manager, std::uint32_t channel) {
    BobusangDeveloperReply reply;
    if (channel >= manager.m_nChannelTotalNum) {
        reply.protocol = CHEAT_BOBUSANG_LEAVE_NACK;
        reply.data = 99u;
        return reply;
    }
    auto* current = bobusang_info(manager, channel, eBBSIT_CUR);
    if (manager.m_AppearedState[channel] != eBBSAS_APPEAR || current == nullptr) {
        reply.protocol = CHEAT_BOBUSANG_LEAVE_NACK;
        reply.data = 9999u;
        return reply;
    }
    if (current->AppearanceMapNum == 0u) {
        reply.protocol = CHEAT_BOBUSANG_LEAVE_NACK;
        reply.data = 999u;
        return reply;
    }
    manager.m_AppearedState[channel] = eBBSAS_DISAPPEAR_DELAYED;
    reply.protocol = CHEAT_BOBUSANG_LEAVE_ACK;
    reply.info[0] = *current;
    return reply;
}

} // namespace mxh::server