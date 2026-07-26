// party_war_mgr.hpp - Phase D5 1:1 port of legacy [Server]Map/PartyWarMgr.h.
// State machine for party-vs-party war: two teams of up to MAX_PARTY_LISTNUM
// members each, locked/ready phases, fight results. Mirrors legacy
// CPartyWarTeam + CPartyWar + CPartyWarMgr fields in CamelCase.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

// ---- Constants 1:1 ----

inline constexpr std::uint32_t MAX_PARTY_LISTNUM = 7u;
inline constexpr std::uint32_t MAX_NAME_LENGTH   = 17u;

// ---- Enumerations ----

enum class PartyWarState : std::uint8_t {
    Null    = 0,
    PreWait = 1,
    Wait    = 2,
    Ready   = 3,
    Fight   = 4,
    Result  = 5,
    End     = 6,
};

// ---- POD structs ----

// Mirrors legacy sPWMember.
struct PWMember {
    std::uint32_t dwMemberIdx = 0;
    bool          bEnableWar   = false;
};

// Opaque POD for legacy VECTOR3.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// Mirrors legacy CPartyWarTeam.
struct PartyWarTeam {
    std::uint32_t                 m_dwPartyIdx = 0;
    std::array<PWMember, MAX_PARTY_LISTNUM> m_Member{};
    std::array<char, MAX_NAME_LENGTH + 1u>  m_sMasterName{};
    int                            m_nAliveNum = 0;
    bool                           m_bLock     = false;
    bool                           m_bReady    = false;
    Vec3                           m_vWarPos{};
};

// Mirrors legacy CPartyWar.
struct PartyWar {
    std::uint32_t m_dwIdx     = 0;
    PartyWarTeam  m_Team1{};
    PartyWarTeam  m_Team2{};
    int           m_nState    = 0;
    std::uint32_t m_dwWarTime = 0;
    std::uint32_t m_dwWinner  = 0;
};

// Mirrors legacy CPartyWarMgr.
struct PartyWarMgrState {
    std::vector<PartyWar>            m_PartyWarTable;
    std::uint32_t                    m_dwPartyWarTableIdx = 1;
};

// ---- Free functions ----

inline PartyWarMgrState make_party_war_mgr() {
    return PartyWarMgrState{};
}

inline void party_war_mgr_init(PartyWarMgrState& s) {
    s.m_PartyWarTable.clear();
    s.m_dwPartyWarTableIdx = 1;
}

inline void party_war_mgr_release(PartyWarMgrState& s) {
    party_war_mgr_init(s);
}

// ---- Team helpers ----

inline void party_war_team_set_party_idx(PartyWarTeam& t, std::uint32_t idx) {
    t.m_dwPartyIdx = idx;
}

inline std::uint32_t party_war_team_get_party_idx(const PartyWarTeam& t) {
    return t.m_dwPartyIdx;
}

inline bool party_war_team_is_alive(const PartyWarTeam& t) {
    return t.m_nAliveNum > 0;
}

inline void party_war_team_set_lock(PartyWarTeam& t, bool b) {
    t.m_bLock = b;
}

inline bool party_war_team_is_locked(const PartyWarTeam& t) {
    return t.m_bLock;
}

inline void party_war_team_set_ready(PartyWarTeam& t, bool b) {
    t.m_bReady = b;
}

inline bool party_war_team_is_ready(const PartyWarTeam& t) {
    return t.m_bReady;
}

inline void party_war_team_set_master_name(PartyWarTeam& t, const char* p_name) {
    if (!p_name) return;
    const std::size_t n = MAX_NAME_LENGTH;
    for (std::size_t i = 0; i < n; ++i) {
        if (p_name[i] == 0) break;
        t.m_sMasterName[i] = p_name[i];
    }
    t.m_sMasterName[n] = 0;
}

inline void party_war_team_set_war_position(PartyWarTeam& t, Vec3 v) {
    t.m_vWarPos = v;
}

inline Vec3 party_war_team_get_war_position(const PartyWarTeam& t) {
    return t.m_vWarPos;
}

// Member init: sets member idx and marks enabled.
inline void party_war_team_init_member(PartyWarTeam& t, std::uint32_t member_idx, int index) {
    if (index < 0 || index >= static_cast<int>(MAX_PARTY_LISTNUM)) return;
    t.m_Member[static_cast<std::size_t>(index)].dwMemberIdx = member_idx;
    t.m_Member[static_cast<std::size_t>(index)].bEnableWar   = true;
    t.m_nAliveNum += 1;
}

inline void party_war_team_add_member(PartyWarTeam& t, std::uint32_t member_idx, int index) {
    party_war_team_init_member(t, member_idx, index);
}

inline bool party_war_team_is_addable_member(const PartyWarTeam& t, std::uint32_t member_idx, int index) {
    if (index < 0 || index >= static_cast<int>(MAX_PARTY_LISTNUM)) return false;
    if (t.m_Member[static_cast<std::size_t>(index)].dwMemberIdx != 0u) return false;
    // Also reject if member already exists in another slot.
    for (std::size_t i = 0; i < MAX_PARTY_LISTNUM; ++i) {
        if (t.m_Member[i].dwMemberIdx == member_idx) return false;
    }
    (void)member_idx;
    return true;
}

inline bool party_war_team_is_war_member(const PartyWarTeam& t, std::uint32_t member_idx) {
    for (std::size_t i = 0; i < MAX_PARTY_LISTNUM; ++i) {
        if (t.m_Member[i].dwMemberIdx == member_idx) return t.m_Member[i].bEnableWar;
    }
    return false;
}

// RemoveMember: zero the slot but keep alive count tied to enable flag.
inline void party_war_team_remove_member(PartyWarTeam& t, std::uint32_t member_idx, int index) {
    if (index < 0 || index >= static_cast<int>(MAX_PARTY_LISTNUM)) return;
    if (t.m_Member[static_cast<std::size_t>(index)].dwMemberIdx != member_idx) return;
    if (t.m_Member[static_cast<std::size_t>(index)].bEnableWar) {
        t.m_nAliveNum -= 1;
        if (t.m_nAliveNum < 0) t.m_nAliveNum = 0;
    }
    t.m_Member[static_cast<std::size_t>(index)] = PWMember{};
}

// MemberDie: returns true if this member was alive before.
inline bool party_war_team_member_die(PartyWarTeam& t, std::uint32_t member_idx) {
    for (std::size_t i = 0; i < MAX_PARTY_LISTNUM; ++i) {
        if (t.m_Member[i].dwMemberIdx == member_idx && t.m_Member[i].bEnableWar) {
            t.m_Member[i].bEnableWar = false;
            t.m_nAliveNum -= 1;
            if (t.m_nAliveNum < 0) t.m_nAliveNum = 0;
            return true;
        }
    }
    return false;
}

// ---- CPartyWar helpers ----

inline void party_war_init(PartyWar& w, std::uint32_t party1_idx, std::uint32_t party2_idx, std::uint32_t dw_idx) {
    w.m_dwIdx = dw_idx;
    w.m_Team1 = PartyWarTeam{};
    w.m_Team2 = PartyWarTeam{};
    w.m_Team1.m_dwPartyIdx = party1_idx;
    w.m_Team2.m_dwPartyIdx = party2_idx;
    w.m_nState    = static_cast<int>(PartyWarState::PreWait);
    w.m_dwWarTime = 0;
    w.m_dwWinner  = 0;
}

inline std::uint32_t party_war_get_index(const PartyWar& w) {
    return w.m_dwIdx;
}

inline int party_war_get_state(const PartyWar& w) {
    return w.m_nState;
}

inline void party_war_get_party_indices(const PartyWar& w, std::uint32_t& p1, std::uint32_t& p2) {
    p1 = w.m_Team1.m_dwPartyIdx;
    p2 = w.m_Team2.m_dwPartyIdx;
}

// IsMemberInPartyWar: returns 1 if in team1, 2 if in team2, 0 if neither.
inline int party_war_is_member(const PartyWar& w, std::uint32_t player_idx, std::uint32_t party_idx) {
    if (party_war_team_is_war_member(w.m_Team1, player_idx) && w.m_Team1.m_dwPartyIdx == party_idx) return 1;
    if (party_war_team_is_war_member(w.m_Team2, player_idx) && w.m_Team2.m_dwPartyIdx == party_idx) return 2;
    return 0;
}

// IsEnemy: two players are enemies iff they're on different teams.
inline bool party_war_is_enemy(const PartyWar& w, std::uint32_t player_idx, std::uint32_t target_idx) {
    int p1_team = 0, p2_team = 0;
    for (int team = 1; team <= 2; ++team) {
        auto& t = (team == 1) ? w.m_Team1 : w.m_Team2;
        if (party_war_team_is_war_member(t, player_idx)) p1_team = team;
        if (party_war_team_is_war_member(t, target_idx)) p2_team = team;
    }
    return (p1_team != 0 && p2_team != 0 && p1_team != p2_team);
}

// PlayerDie: returns true if this player was alive.
inline bool party_war_player_die(PartyWar& w, std::uint32_t player_idx, std::uint32_t party_idx) {
    if (w.m_Team1.m_dwPartyIdx == party_idx) {
        return party_war_team_member_die(w.m_Team1, player_idx);
    }
    if (w.m_Team2.m_dwPartyIdx == party_idx) {
        return party_war_team_member_die(w.m_Team2, player_idx);
    }
    return false;
}

// RemovePlayer
inline void party_war_remove_player(PartyWar& w, std::uint32_t player_idx, std::uint32_t party_idx) {
    if (w.m_Team1.m_dwPartyIdx == party_idx) {
        for (std::size_t i = 0; i < MAX_PARTY_LISTNUM; ++i) {
            if (w.m_Team1.m_Member[i].dwMemberIdx == player_idx) {
                party_war_team_remove_member(w.m_Team1, player_idx, static_cast<int>(i));
                return;
            }
        }
    } else if (w.m_Team2.m_dwPartyIdx == party_idx) {
        for (std::size_t i = 0; i < MAX_PARTY_LISTNUM; ++i) {
            if (w.m_Team2.m_Member[i].dwMemberIdx == player_idx) {
                party_war_team_remove_member(w.m_Team2, player_idx, static_cast<int>(i));
                return;
            }
        }
    }
}

// Process tick advances the war's state machine.
inline bool party_war_process(PartyWar& w, std::uint32_t elapsed_ms) {
    switch (static_cast<PartyWarState>(w.m_nState)) {
        case PartyWarState::PreWait:
            w.m_dwWarTime += elapsed_ms;
            if (w.m_dwWarTime >= 10000u) { w.m_nState = static_cast<int>(PartyWarState::Wait); return true; }
            break;
        case PartyWarState::Wait:
            if (party_war_team_is_locked(w.m_Team1) && party_war_team_is_locked(w.m_Team2)) {
                w.m_nState = static_cast<int>(PartyWarState::Ready);
                return true;
            }
            break;
        case PartyWarState::Ready:
            if (party_war_team_is_ready(w.m_Team1) && party_war_team_is_ready(w.m_Team2)) {
                w.m_nState = static_cast<int>(PartyWarState::Fight);
                return true;
            }
            break;
        case PartyWarState::Fight:
            if (w.m_Team1.m_nAliveNum == 0) { w.m_dwWinner = 2; w.m_nState = static_cast<int>(PartyWarState::Result); return true; }
            if (w.m_Team2.m_nAliveNum == 0) { w.m_dwWinner = 1; w.m_nState = static_cast<int>(PartyWarState::Result); return true; }
            break;
        case PartyWarState::Result:
            w.m_nState = static_cast<int>(PartyWarState::End);
            return true;
        default:
            break;
    }
    return false;
}

// ---- CPartyWarMgr ----

inline std::optional<std::uint32_t> register_party_war(PartyWarMgrState& s,
                                                     std::uint32_t party1_idx,
                                                     std::uint32_t party2_idx) {
    PartyWar w;
    party_war_init(w, party1_idx, party2_idx, s.m_dwPartyWarTableIdx);
    s.m_PartyWarTable.push_back(w);
    const std::uint32_t idx = s.m_dwPartyWarTableIdx;
    s.m_dwPartyWarTableIdx += 1;
    return idx;
}

inline PartyWar* find_party_war_by_id(PartyWarMgrState& s, std::uint32_t idx) {
    for (auto& w : s.m_PartyWarTable) {
        if (w.m_dwIdx == idx) return &w;
    }
    return nullptr;
}

inline bool unregister_party_war(PartyWarMgrState& s, std::uint32_t idx) {
    for (auto it = s.m_PartyWarTable.begin(); it != s.m_PartyWarTable.end(); ++it) {
        if (it->m_dwIdx == idx) {
            s.m_PartyWarTable.erase(it);
            return true;
        }
    }
    return false;
}

} // namespace mxh::server
