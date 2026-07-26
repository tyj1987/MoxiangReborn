// survival_mode_manager.hpp - Phase D5 1:1 port of legacy [Server]Map/SurvivalModeManager.h.
// Manages the in-game Survival (free-for-all) mode: state machine, user
// alive count, item using counter, and timing transitions.

#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mxh::server {

// ---- Constants 1:1 ----

inline constexpr std::uint32_t SVVMOD_TIME_READY = 15000u;
inline constexpr std::uint32_t SVVMOD_TIME_END   = 10000u;

// ---- Enumerations ----

enum class SurvivalModeState : std::uint16_t {
    None  = 0,
    Ready = 1,
    Fight = 2,
    End   = 3,
};

// ---- POD state ----

// Mirrors legacy CSurvivalModeManager.
struct SurvivalModeManagerState {
    std::uint16_t m_wModeState = 0;
    std::uint32_t m_dwStateRemainTime = 0;
    std::uint32_t m_dwUsingCountLimit = 0;
    int           m_nUserAlive = 0;
    std::unordered_set<std::uint32_t> m_SVModeUserTable;   // opaque CObject*
    std::unordered_set<std::uint32_t> m_SVModeAliveUserList;
    // For each user, count of item uses (player_id -> use count).
    std::unordered_map<std::uint32_t, std::uint32_t> m_SVItemUsingCounter;
};

// ---- Free functions ----

inline SurvivalModeManagerState make_survival_manager() {
    return SurvivalModeManagerState{};
}

inline void survival_init(SurvivalModeManagerState& s) {
    s.m_wModeState = static_cast<std::uint16_t>(SurvivalModeState::None);
    s.m_dwStateRemainTime = 0;
    s.m_dwUsingCountLimit = 0;
    s.m_nUserAlive = 0;
    s.m_SVModeUserTable.clear();
    s.m_SVModeAliveUserList.clear();
    s.m_SVItemUsingCounter.clear();
}

inline void survival_release(SurvivalModeManagerState& s) {
    survival_init(s);
}

// SetCurModeState.
inline void set_cur_mode_state(SurvivalModeManagerState& s, SurvivalModeState st) {
    s.m_wModeState = static_cast<std::uint16_t>(st);
}

inline SurvivalModeState get_cur_mode_state(const SurvivalModeManagerState& s) {
    return static_cast<SurvivalModeState>(s.m_wModeState);
}

// ChangeStateTo: legacy entry-point; updates state + sets up timer.
inline void change_state_to(SurvivalModeManagerState& s, SurvivalModeState next) {
    s.m_wModeState = static_cast<std::uint16_t>(next);
    switch (next) {
        case SurvivalModeState::Ready: s.m_dwStateRemainTime = SVVMOD_TIME_READY; break;
        case SurvivalModeState::End:   s.m_dwStateRemainTime = SVVMOD_TIME_END;   break;
        case SurvivalModeState::Fight: s.m_dwStateRemainTime = 0; break;
        default:                       s.m_dwStateRemainTime = 0; break;
    }
}

// Process tick: decrements remaining time; transitions Ready->Fight, Fight->End, End->None.
inline bool survival_tick(SurvivalModeManagerState& s, std::uint32_t elapsed_ms) {
    if (s.m_dwStateRemainTime == 0u) return false;
    if (elapsed_ms >= s.m_dwStateRemainTime) {
        s.m_dwStateRemainTime = 0;
        switch (static_cast<SurvivalModeState>(s.m_wModeState)) {
            case SurvivalModeState::Ready: change_state_to(s, SurvivalModeState::Fight); break;
            case SurvivalModeState::Fight: change_state_to(s, SurvivalModeState::End);   break;
            case SurvivalModeState::End:   change_state_to(s, SurvivalModeState::None);  break;
            default: break;
        }
        return true;
    }
    s.m_dwStateRemainTime -= elapsed_ms;
    return false;
}

// CheckRemainTime: returns true if timer is set (active).
inline bool check_remain_time(const SurvivalModeManagerState& s) {
    return s.m_dwStateRemainTime > 0u;
}

// User lifecycle.
inline void add_sv_mode_user(SurvivalModeManagerState& s, std::uint32_t obj_id) {
    s.m_SVModeUserTable.insert(obj_id);
}

inline void remove_sv_mode_user(SurvivalModeManagerState& s, std::uint32_t obj_id) {
    s.m_SVModeUserTable.erase(obj_id);
    const auto n = s.m_SVModeAliveUserList.erase(obj_id);
    if (n > 0u) s.m_nUserAlive -= 1;
}

inline bool is_sv_mode_user(const SurvivalModeManagerState& s, std::uint32_t obj_id) {
    return s.m_SVModeUserTable.count(obj_id) > 0u;
}

inline std::size_t sv_mode_user_count(const SurvivalModeManagerState& s) {
    return s.m_SVModeUserTable.size();
}

// Alive count.
inline void add_alive_user(SurvivalModeManagerState& s, std::uint32_t obj_id) {
    if (s.m_SVModeUserTable.count(obj_id) == 0u) return;
    const bool inserted = s.m_SVModeAliveUserList.insert(obj_id).second;
    if (inserted) s.m_nUserAlive += 1;
}

inline void remove_alive_user(SurvivalModeManagerState& s, std::uint32_t obj_id) {
    const auto n = s.m_SVModeAliveUserList.erase(obj_id);
    if (n > 0u) s.m_nUserAlive -= 1;
}

inline int get_alive_user_count(const SurvivalModeManagerState& s) {
    return s.m_nUserAlive;
}

// Item using limit.
inline void set_using_count_limit(SurvivalModeManagerState& s, std::uint32_t limit) {
    s.m_dwUsingCountLimit = limit;
}

inline std::uint32_t get_using_count_limit(const SurvivalModeManagerState& s) {
    return s.m_dwUsingCountLimit;
}

// AddItemUsingCount: increments per-user counter; returns true if the new
// count is within the limit (so caller can permit the action).
inline bool add_item_using_count(SurvivalModeManagerState& s, std::uint32_t player_id) {
    std::uint32_t next = 0;
    auto it = s.m_SVItemUsingCounter.find(player_id);
    if (it == s.m_SVItemUsingCounter.end()) {
        next = 1u;
        s.m_SVItemUsingCounter[player_id] = next;
    } else {
        it->second += 1u;
        next = it->second;
    }
    if (s.m_dwUsingCountLimit == 0u) return true;  // 0 = unlimited
    return next <= s.m_dwUsingCountLimit;
}

inline std::uint32_t get_item_using_count(const SurvivalModeManagerState& s, std::uint32_t player_id) {
    auto it = s.m_SVItemUsingCounter.find(player_id);
    return (it == s.m_SVItemUsingCounter.end()) ? 0u : it->second;
}

// ReadyToSurvivalMode: legacy entry-point. Sets Ready + 15s timer.
inline void ready_to_survival_mode(SurvivalModeManagerState& s) {
    change_state_to(s, SurvivalModeState::Ready);
}

// ReturnToMap: legacy entry-point. Sets state to None, clears alive count.
inline void return_to_map(SurvivalModeManagerState& s) {
    s.m_nUserAlive = 0;
    s.m_SVModeAliveUserList.clear();
    change_state_to(s, SurvivalModeState::None);
}

} // namespace mxh::server
