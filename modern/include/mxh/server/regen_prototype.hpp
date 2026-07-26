// regen_prototype.hpp - Phase D6 RegenPrototype 1:1 port.
//
// Source-of-truth: legacy [Server]Map/RegenPrototype.h + .cpp.
// Mirrors legacy CRegenPrototype (regen slot metadata) and
// CRegenObject (live instance) PODs.  The VECTOR3 type is the
// legacy 12-byte struct; modern exposes it as three floats for
// compatibility while retaining the same field ordering.

#pragma once

#include <cstdint>

namespace mxh::server {

// Mirror of legacy VECTOR3 (3 floats, 12 bytes).
struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// Mirror of legacy CRegenPrototype.
struct RegenPrototype {
    std::uint8_t  RegenType         = 0;
    std::uint8_t  ObjectKind        = 0;
    std::uint16_t wMonsterKind      = 0;
    std::uint32_t dwObjectID        = 0;
    Vector3       vPos              = {};
    std::uint16_t InitHelpType      = 0;
    bool          bHearing          = false;
    std::uint32_t HearingDistance   = 0;
};

// Mirror of legacy CRegenObject.
struct RegenObject {
    std::uint32_t m_dwObjectID     = 0;
    std::uint32_t m_dwSubObjectID  = 0;
    std::uint32_t m_dwGridID       = 0;
    std::uint32_t m_dwGroupID      = 0;
    std::uint16_t m_CurHelpType    = 0;
    RegenPrototype* m_pPrototype   = nullptr;

    // Field-style accessors mirror the legacy inline getters.
    bool     has_prototype() const { return m_pPrototype != nullptr; }
    bool     is_hearing()    const { return has_prototype() && m_pPrototype->bHearing; }
    std::uint8_t  get_object_kind() const { return m_pPrototype->ObjectKind; }
    std::uint16_t get_monster_kind() const { return m_pPrototype->wMonsterKind; }
    const Vector3* get_pos() const { return &m_pPrototype->vPos; }
    std::uint32_t get_hearing_distance() const { return m_pPrototype->HearingDistance; }
    std::uint16_t get_cur_help_type() const { return m_CurHelpType; }
    std::uint32_t get_sub_id() const { return m_dwSubObjectID; }
    std::uint32_t get_group_id() const { return m_dwGroupID; }

    void set_cur_help_type(std::uint16_t type) { m_CurHelpType = type; }
};

// InitPrototype: copy the pointer into m_pPrototype.
void regen_object_init_prototype(RegenObject& obj, RegenPrototype* prototype);

// InitHelpType: copy InitHelpType from prototype to m_CurHelpType.
void regen_object_init_help_type(RegenObject& obj);

}  // namespace mxh::server
