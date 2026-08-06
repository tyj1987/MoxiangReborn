// dup_param.hpp
//
// 1:1 port of legacy CShopItemManager::AddDupParam / DeleteDupParam /
// IsDupAble from [Server]Map/ShopItemManager.cpp:2286-2620. Splits the
// legacy per-bit OR / XOR-when-both-set / check blocker logic into a
// pure data plane (this header) + an orchestrator half that handles the
// SHOPITEMDUP table lookup and the ShopItemStats bStreetStall side
// effects.
//
// 1:1 invariants:
//   - The "dup-able" blocks are 5 (charm / herb / incantation /
//     sundries / pet-equip).
//   - Each block reads a different field of the legacy ITEM_INFO:
//       charm         AllPlus_Value       -> SHOPITEMDUP table
//       herb          MugongNum           -> SHOPITEMDUP table
//       incantation   MugongType          -> SHOPITEMDUP table
//       sundries      LifeRecover (DWORD) -> SHOPITEMDUP table
//       pet_equip     LifeRecoverRate (float) -> SHOPITEMDUP table
//   - The dup counter byte is 1:1 with m_DupCharm / m_DupHerb /
//     m_DupIncantation / m_DupSundries / m_DupPetEquip (legacy DWORD).
//   - AddDupParam: OR the dup-table Param bits into the counter.
//   - DeleteDupParam: XOR (clear) the dup-table Param bits, but only if
//     the corresponding counter bit is set (legacy: `(pDupOption->Param
//     & FLAG) && (m_DupXXX & FLAG)` then XOR).
//   - IsDupAble: return true if NONE of the dup-table Param bits are
//     already set in the counter (legacy: returns FALSE if any are set).
//   - The bStreetStall side effect (legacy AddDupParam: bStreetStall=1,
//     DeleteDupParam: bStreetStall=0) is captured in the sundry_bits
//     struct so the orchestrator can dispatch.
//
// The DupParam field is a 32-bit bitset. The legacy flag values are
// powers of two (2, 4, 8, ..., 256) and they overlap across the 5
// categories (a charm bit and a herb bit can have the same numeric
// value). The data plane keeps the categories separate but exposes
// the bitset as a single uint32 per category.

#pragma once

#include <cstdint>

namespace mxh::server {

// ============================================================================
// Dup tables per category. Each enum defines the eDontDupUse_* flag values
// the corresponding dup counter can contain. The numeric values are 1:1
// with [CC]Header/CommonGameDefine.h:2874-2930 (DONTDUP_INCANTATION /
// DONTDUP_CHARM / DONTDUP_HERB / DONTDUP_SUNDRIES / DONTDUP_PETEQUIP).
// ============================================================================

namespace charm_dup {
    inline constexpr std::uint32_t WoigongDamage = 2;
    inline constexpr std::uint32_t NaegongDamage = 4;
    inline constexpr std::uint32_t Exppoint      = 8;
    inline constexpr std::uint32_t Reinforce     = 16;
    inline constexpr std::uint32_t Kyunggong     = 32;
    inline constexpr std::uint32_t Ghost         = 64;     // ¿À°³ÁÖ¹®¼­
    inline constexpr std::uint32_t Woigong       = 128;    // ¿Ü°øÁÖ¹®¼­
    inline constexpr std::uint32_t Naegong       = 256;    // ³»°øÁÖ¹®¼­
    inline constexpr std::uint32_t Hunter        = 16384;  // »ç³É²Ù Á¶ÀÛ
    inline constexpr std::uint32_t ExpDay        = 32768;  // °æÇèÄ¡ Á¶
}

namespace herb_dup {
    inline constexpr std::uint32_t Life           = 2;
    inline constexpr std::uint32_t Shield         = 4;
    inline constexpr std::uint32_t Naeruyk        = 8;
    inline constexpr std::uint32_t GreateLife     = 16;
    inline constexpr std::uint32_t GreateShield   = 32;
    inline constexpr std::uint32_t GreateNaeruyk  = 64;
    inline constexpr std::uint32_t EventSatang    = 128;
    inline constexpr std::uint32_t Doll           = 256;
}

namespace incantation_dup {
    inline constexpr std::uint32_t MemoryMove     = 2;
    inline constexpr std::uint32_t ProtectAll     = 4;
    inline constexpr std::uint32_t LevelCancel50  = 8;
    inline constexpr std::uint32_t LevelCancel70  = 16;
    inline constexpr std::uint32_t LevelCancel90  = 32;
    inline constexpr std::uint32_t ShowPyoguk     = 64;
    inline constexpr std::uint32_t Chase          = 128;
    inline constexpr std::uint32_t TownMove       = 256;
}

namespace sundries_dup {
    inline constexpr std::uint32_t StreetStall    = 2;
}

namespace pet_equip_dup {
    inline constexpr std::uint32_t PomanRing      = 2;
}

// ============================================================================
// Dup counters. Each counter is a 32-bit bitset that accumulates the eDontDupUse_*
// flags from the SHOPITEMDUP table. The 5 counters mirror the legacy
// CShopItemManager members.
// ============================================================================

struct DupCounters {
    std::uint32_t charm        = 0;
    std::uint32_t herb         = 0;
    std::uint32_t incantation  = 0;
    std::uint32_t sundries     = 0;
    std::uint32_t pet_equip    = 0;
};

// Helper accessors for the 5 dup counters. Used by the orchestrator
// when wiring the ShopItemManager member field types.
inline std::uint32_t& dup_charm(DupCounters& c)       noexcept { return c.charm; }
inline std::uint32_t& dup_herb(DupCounters& c)        noexcept { return c.herb; }
inline std::uint32_t& dup_incantation(DupCounters& c) noexcept { return c.incantation; }
inline std::uint32_t& dup_sundries(DupCounters& c)    noexcept { return c.sundries; }
inline std::uint32_t& dup_pet_equip(DupCounters& c)   noexcept { return c.pet_equip; }

// ============================================================================
// 1:1 with legacy ITEM_INFO's dup-table lookup fields. The modern port
// factors the 5 fields into one struct so the orchestrator can pass all
// 5 SHOPITEMDUP table indices in one call.
// ============================================================================

struct DupParamIndices {
    std::uint32_t all_plus_value    = 0;  // 0 means no charm SHOPITEMDUP
    std::uint32_t mugong_num        = 0;  // 0 means no herb SHOPITEMDUP
    std::uint32_t mugong_type       = 0;  // 0 means no incantation SHOPITEMDUP
    std::uint32_t life_recover      = 0;  // 0 means no sundries SHOPITEMDUP
    std::uint32_t life_recover_rate = 0;  // 0 means no pet-equip SHOPITEMDUP
};

// ============================================================================
// SHOPITEMDUP table lookup callback. The legacy code reads from
// GAMERESRCMNGR->m_ShopItemDupOptionTable (DWORD key -> SHOPITEMDUP*).
// The data plane leaves the lookup to the orchestrator (a virtual call
// rather than a global) so the data plane can be tested without a
// global resource table.
// ============================================================================

class DupParamLookup {
public:
    virtual ~DupParamLookup() = default;
    // Returns the SHOPITEMDUP.Param bitset for the given index, or 0
    // if no entry exists in the legacy table. The default implementation
    // returns 0 (perfectly valid for the "no dup protection" case).
    virtual std::uint32_t dup_param_for(std::uint32_t index) const noexcept {
        (void)index;
        return 0;
    }
};

// ============================================================================
// Side effects of the sundries bit (eDontDupUse_StreeStall). The legacy
// code toggles m_pPlayer->GetShopItemStats()->bStreetStall when this
// bit is set / cleared. The data plane captures the toggle in this
// struct so the orchestrator can dispatch the player-side update.
// ============================================================================

struct SundrySideEffects {
    bool set_b_street_stall   = false;  // true if bStreetStall should be 1
    bool clear_b_street_stall = false;  // true if bStreetStall should be 0
};

// 1:1 with legacy CShopItemManager::AddDupParam. For each of the 5
// category blocks: lookup the SHOPITEMDUP table by the corresponding
// index, then OR the dup.Param bits into the appropriate counter.
// Sundries has a side effect on the player's bStreetStall (captured in
// sundry_side_effects).
void add_dup_param(DupCounters& counters,
                   const DupParamIndices& indices,
                   const DupParamLookup& lookup,
                   SundrySideEffects& sundry_side_effects) noexcept;

// 1:1 with legacy CShopItemManager::DeleteDupParam. For each of the 5
// category blocks: lookup the SHOPITEMDUP table by the corresponding
// index, then XOR (clear) the dup.Param bits from the counter, but
// only if the corresponding counter bit is set. Sundries has a side
// effect on the player's bStreetStall (captured in
// sundry_side_effects).
void delete_dup_param(DupCounters& counters,
                      const DupParamIndices& indices,
                      const DupParamLookup& lookup,
                      SundrySideEffects& sundry_side_effects) noexcept;

// 1:1 with legacy CShopItemManager::IsDupAble. Returns true if NONE of
// the dup.Param bits in any of the 5 categories are already set in the
// counter. Legacy returns FALSE as soon as any overlap is detected.
bool is_dup_able(const DupCounters& counters,
                 const DupParamIndices& indices,
                 const DupParamLookup& lookup) noexcept;

}  // namespace mxh::server
