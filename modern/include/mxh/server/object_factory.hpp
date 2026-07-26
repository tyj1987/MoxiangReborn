// object_factory.hpp - Phase D6 ObjectFactory 1:1 port (subset).
//
// Source-of-truth: legacy [Server]Map/ObjectFactory.h + .cpp.
// Mirrors the per-kind object pool counters.  The full pool
// allocator (CMemoryPoolTempl) is replaced by a counter + active
// flag list so the server framework can dispatch MakeNewObject /
// ReleaseObject without owning a 10-pool graph.
//
// Constants mirror the legacy MAX_TOTAL_*_NUM caps.

#pragma once

#include <array>
#include <cstdint>

namespace mxh::server {

// ---- Pool caps (mirror legacy MAX_TOTAL_*_NUM) ----
inline constexpr std::uint32_t MAX_TOTAL_PLAYER_NUM        = 1500u;
inline constexpr std::uint32_t MAX_TOTAL_PET_NUM           = 500u;
inline constexpr std::uint32_t MAX_TOTAL_MONSTER_NUM       = 4000u;
inline constexpr std::uint32_t MAX_TOTAL_BOSSMONSTER_NUM   = 200u;
inline constexpr std::uint32_t MAX_TOTAL_NPC_NUM           = 200u;
inline constexpr std::uint32_t MAX_TOTAL_TACTIC_NUM        = 100u;
inline constexpr std::uint32_t MAX_MAPOBJECT_NUM           = 4000u;
inline constexpr std::uint32_t MAX_TOTAL_TITAN_NUM         = 1500u;
inline constexpr std::uint32_t MAX_TITANINFO_NUM           = 1500u;

// ---- Per-kind index in the counter table ----
enum class ObjectKind : std::uint8_t {
    Player            = 0,
    Pet               = 1,
    Monster           = 2,
    Npc               = 3,
    Tactic            = 4,
    BossMonster       = 5,
    MapObject         = 6,
    Titan             = 7,
    TitanInfo         = 8,
    FieldBossMonster  = 9,
    FieldSubMonster   = 10,
    Count             = 11,
};

// ---- Active counter table ----
struct ObjectFactoryState {
    std::array<std::uint32_t, static_cast<std::size_t>(ObjectKind::Count)> active{};
    std::array<std::uint32_t, static_cast<std::size_t>(ObjectKind::Count)> cap{};
    bool initialized = false;

    std::uint32_t total_active() const;
    std::uint32_t total_cap() const;
};

// Initialize the factory with the legacy per-kind caps.
void object_factory_init(ObjectFactoryState& s);

// Tear down: legacy calls Release() which deletes all pools.  Modern
// just clears the counters and marks the factory as uninitialized.
void object_factory_release(ObjectFactoryState& s);

// Returns true if a new object of the given kind can be created
// without exceeding the cap.
bool object_factory_can_make(const ObjectFactoryState& s, ObjectKind kind);

// Records the creation of a new object.  Returns false if the cap
// was exceeded (caller must NOT proceed with allocation in that case).
bool object_factory_make_new(ObjectFactoryState& s, ObjectKind kind);

// Records the release of an existing object.  Saturates at zero.
void object_factory_release_object(ObjectFactoryState& s, ObjectKind kind);

// ---- Result enum for the wire-level protocol hook ----
enum class MakeObjectResult : std::uint8_t {
    Created      = 0,
    PoolFull     = 1,
    NotInit      = 2,
    UnknownKind  = 3,
};

MakeObjectResult object_factory_make_new_ex(ObjectFactoryState& s,
                                            std::uint8_t kind_byte);

}  // namespace mxh::server
