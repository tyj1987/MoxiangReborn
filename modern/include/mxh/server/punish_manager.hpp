// punish_manager.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/PunishManager.h + PunishManager.cpp (CPunishManager + CPunishUnit).
//
// PunishManager tracks per-user punishment timers (login / autonote / chat /
// trade) so the AgentServer can throttle a misbehaving user without DB
// round-trips. Punishments expire when their EndTime elapses.
//
// Locked invariants (1:1 with legacy):
//   - PunishKind enum values match legacy (ePunish_Login = 0,
//     ePunish_AutoNoteUse = 1, ePunish_Chat = 2, ePunish_Trade = 3,
//     ePunish_Max = 4). The two values stored in the DB are these enum
//     numbers; modern must keep them stable so RPunishListLoad can map
//     them back.
//   - AddPunishUnit stores m_dwEndTime = now + seconds*1000 (legacy
//     multiplies the seconds input by 1000 to get milliseconds).
//   - AddPunishUnit on an existing (user, kind) pair overwrites the
//     EndTime; legacy calls Init() again rather than freeing.
//   - IsTimeEnd returns TRUE iff now > m_dwEndTime.
//   - GetRemainTime returns 0 when now > m_dwEndTime, else m_dwEndTime -
//     now. (legacy uses DWORD arithmetic; the wrap is benign because
//     the IsTimeEnd precheck guarantees we only subtract when now <=
//     EndTime.)
//   - AutoNoteUseTime and AutoBlockTime default to 60 minutes (legacy
//     Init()).
//
// Out of scope for this port:
//   - Legacy Process() sweeps all four hash tables every 1 second to GC
//     expired units; modern exposes sweep_expired(now_ms) so tests can
//     drive time deterministically. The caller is expected to invoke it
//     at 1-second intervals.
//   - gCurTime / CMemoryPoolTempl / CYHHashTable: modern uses plain
//     unordered_map<std::uint32_t, std::unique_ptr<PunishUnit>> per kind
//     and std::vector<std::unique_ptr> for memory ownership.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mxh::server {

// ---- Enums 1:1 with legacy ePunishKind ----

enum class PunishKind : int {
    ePunish_Login      = 0,
    ePunish_AutoNoteUse = 1,
    ePunish_Chat       = 2,
    ePunish_Trade      = 3,
    ePunish_Max        = 4,
};

// ---- POD (1:1 with legacy CPunishUnit) ----

struct PunishUnit {
    std::uint32_t dwUserIdx    = 0;
    int           nPunishKind  = 0;     // legacy stores kind index here too
    std::uint32_t dwEndTime    = 0;     // ms epoch
};

// Mirrors legacy CPunishManager state.
struct PunishManager {
    std::array<std::unordered_map<std::uint32_t, std::unique_ptr<PunishUnit>>,
                static_cast<std::size_t>(PunishKind::ePunish_Max)>
        m_htPunishUnit;
    std::vector<std::unique_ptr<PunishUnit>> m_Owned;  // backing storage
    std::uint32_t m_dwAutoNoteUseTime = 60u;  // minutes (legacy default)
    std::uint32_t m_dwAutoBlockTime   = 60u;  // minutes (legacy default)
};

// ---- Lifecycle ----

inline PunishManager make_punish_manager() {
    return PunishManager{};
}

inline void punish_manager_init(PunishManager& m) {
    for (auto& t : m.m_htPunishUnit) t.clear();
    m.m_Owned.clear();
    m.m_dwAutoNoteUseTime = 60u;
    m.m_dwAutoBlockTime   = 60u;
}

inline void punish_manager_release(PunishManager& m) {
    m.m_Owned.clear();
    for (auto& t : m.m_htPunishUnit) t.clear();
}

// ---- Unit helpers (mirrors legacy CPunishUnit::Init / IsTimeEnd /
// GetRemainTime; modern takes `now_ms` as a parameter) ----

inline void punish_unit_init(PunishUnit& u,
                              std::uint32_t dwUserIdx,
                              int nPunishKind,
                              std::uint32_t dwEndTime) {
    u.dwUserIdx   = dwUserIdx;
    u.nPunishKind = nPunishKind;
    u.dwEndTime   = dwEndTime;
}

inline bool punish_unit_is_time_end(const PunishUnit& u, std::uint32_t now_ms) {
    return now_ms > u.dwEndTime;
}

inline std::uint32_t punish_unit_get_remain_time(const PunishUnit& u,
                                                  std::uint32_t now_ms) {
    if (now_ms > u.dwEndTime) return 0u;
    return u.dwEndTime - now_ms;
}

// ---- Manager ops ----

inline PunishUnit* get_punish_unit(PunishManager& m,
                                   std::uint32_t dwUserIdx,
                                   PunishKind kind) {
    auto idx = static_cast<std::size_t>(kind);
    if (idx >= m.m_htPunishUnit.size()) return nullptr;
    auto& t = m.m_htPunishUnit[idx];
    auto it = t.find(dwUserIdx);
    return it == t.end() ? nullptr : it->second.get();
}

inline const PunishUnit* get_punish_unit(const PunishManager& m,
                                         std::uint32_t dwUserIdx,
                                         PunishKind kind) {
    auto idx = static_cast<std::size_t>(kind);
    if (idx >= m.m_htPunishUnit.size()) return nullptr;
    const auto& t = m.m_htPunishUnit[idx];
    auto it = t.find(dwUserIdx);
    return it == t.end() ? nullptr : it->second.get();
}

// AddPunishUnit: legacy semantics: dwPunishTime is in seconds, multiplied
// by 1000 to get the millisecond EndTime relative to `now_ms`. If a unit
// already exists for (user, kind), its EndTime is overwritten (Init again).
inline void add_punish_unit(PunishManager& m,
                             std::uint32_t dwUserIdx,
                             PunishKind kind,
                             std::uint32_t dwPunishTime,
                             std::uint32_t now_ms) {
    auto idx = static_cast<std::size_t>(kind);
    if (idx >= m.m_htPunishUnit.size()) return;
    auto& t = m.m_htPunishUnit[idx];
    std::uint32_t milisec = dwPunishTime * 1000u;
    auto it = t.find(dwUserIdx);
    if (it != t.end()) {
        punish_unit_init(*it->second, dwUserIdx, static_cast<int>(kind),
                         now_ms + milisec);
        return;
    }
    auto u = std::make_unique<PunishUnit>();
    punish_unit_init(*u, dwUserIdx, static_cast<int>(kind), now_ms + milisec);
    t.emplace(dwUserIdx, std::move(u));
}

inline bool remove_punish_unit(PunishManager& m,
                                std::uint32_t dwUserIdx,
                                PunishKind kind) {
    auto idx = static_cast<std::size_t>(kind);
    if (idx >= m.m_htPunishUnit.size()) return false;
    auto& t = m.m_htPunishUnit[idx];
    auto it = t.find(dwUserIdx);
    if (it == t.end()) return false;
    t.erase(it);
    return true;
}

inline void remove_punish_unit_all(PunishManager& m,
                                    std::uint32_t dwUserIdx) {
    for (auto& t : m.m_htPunishUnit) {
        auto it = t.find(dwUserIdx);
        if (it != t.end()) t.erase(it);
    }
}

// sweep_expired: legacy Process() walks each kind every 1 second and
// removes the first expired unit per kind (legacy uses `break` after one
// removal per kind per pass). Modern matches that single-removal-per-pass
// behavior so callers can drive the same per-second cadence.
inline std::size_t sweep_expired(PunishManager& m, std::uint32_t now_ms) {
    std::size_t removed = 0;
    for (auto& t : m.m_htPunishUnit) {
        for (auto it = t.begin(); it != t.end(); ++it) {
            if (punish_unit_is_time_end(*it->second, now_ms)) {
                t.erase(it);
                ++removed;
                break;  // legacy: one removal per kind per pass.
            }
        }
    }
    return removed;
}

// ---- AutoNoteUseTime / AutoBlockTime ----

inline std::uint32_t get_auto_note_use_time(const PunishManager& m) {
    return m.m_dwAutoNoteUseTime;
}
inline void set_auto_note_use_time(PunishManager& m, std::uint32_t v) {
    m.m_dwAutoNoteUseTime = v;
}
inline std::uint32_t get_auto_block_time(const PunishManager& m) {
    return m.m_dwAutoBlockTime;
}
inline void set_auto_block_time(PunishManager& m, std::uint32_t v) {
    m.m_dwAutoBlockTime = v;
}

inline std::size_t punish_total_count(const PunishManager& m) {
    std::size_t n = 0;
    for (const auto& t : m.m_htPunishUnit) n += t.size();
    return n;
}

} // namespace mxh::server

