// dup_param.cpp
//
// Pure data plane implementation of CShopItemManager::AddDupParam /
// DeleteDupParam / IsDupAble. See dup_param.hpp for the 1:1 invariants
// and the legacy-line citations.

#include "mxh/server/dup_param.hpp"

namespace mxh::server {

namespace {

// Apply a per-bit OR step. Used by AddDupParam. The legacy code only
// sets bits when the dup-table Param has the corresponding bit set;
// the counter bit is irrelevant on Add (the OR is idempotent).
inline std::uint32_t apply_or(std::uint32_t counter, std::uint32_t dup_param) noexcept {
    return counter | dup_param;
}

// Apply a per-bit XOR-when-both-set step. Used by DeleteDupParam. The
// legacy code only XORs the bit when both the dup-table Param AND the
// counter bit are set; otherwise the bit is left alone.
inline std::uint32_t apply_delete(std::uint32_t counter, std::uint32_t dup_param) noexcept {
    const std::uint32_t overlap = counter & dup_param;
    return counter ^ overlap;
}

}  // namespace

void add_dup_param(DupCounters& counters,
                   const DupParamIndices& indices,
                   const DupParamLookup& lookup,
                   SundrySideEffects& sundry_side_effects) noexcept {
    sundry_side_effects.set_b_street_stall = false;
    sundry_side_effects.clear_b_street_stall = false;

    // Charm block: AllPlus_Value -> SHOPITEMDUP.Param -> OR into Charm counter.
    if (indices.all_plus_value != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.all_plus_value, dup_param)) return;
        constexpr auto mask = charm_dup::WoigongDamage | charm_dup::NaegongDamage |
            charm_dup::Exppoint | charm_dup::Reinforce | charm_dup::Kyunggong |
            charm_dup::Ghost | charm_dup::Woigong | charm_dup::Naegong |
            charm_dup::Hunter | charm_dup::ExpDay;
        counters.charm = apply_or(counters.charm, dup_param & mask);
    }

    // Herb block: MugongNum -> SHOPITEMDUP.Param -> OR into Herb counter.
    if (indices.mugong_num != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.mugong_num, dup_param)) return;
        constexpr auto mask = herb_dup::Life | herb_dup::Shield | herb_dup::Naeruyk |
            herb_dup::GreateLife | herb_dup::GreateShield | herb_dup::GreateNaeruyk |
            herb_dup::EventSatang | herb_dup::Doll;
        counters.herb = apply_or(counters.herb, dup_param & mask);
    }

    // Incantation block: MugongType -> SHOPITEMDUP.Param -> OR into Incantation counter.
    if (indices.mugong_type != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.mugong_type, dup_param)) return;
        constexpr auto mask = incantation_dup::MemoryMove | incantation_dup::ProtectAll |
            incantation_dup::LevelCancel50 | incantation_dup::LevelCancel70 |
            incantation_dup::LevelCancel90 | incantation_dup::ShowPyoguk |
            incantation_dup::Chase | incantation_dup::TownMove;
        counters.incantation = apply_or(counters.incantation, dup_param & mask);
    }

    // Sundries block: LifeRecover -> SHOPITEMDUP.Param -> OR into Sundries
    // counter; if the StreetStall bit is set, the player-side bStreetStall
    // flag should be set to 1.
    if (indices.life_recover != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.life_recover, dup_param)) return;
        counters.sundries = apply_or(counters.sundries, dup_param & sundries_dup::StreetStall);
        if (dup_param & sundries_dup::StreetStall) {
            sundry_side_effects.set_b_street_stall = true;
        }
    }

    // Pet-equip block: LifeRecoverRate -> SHOPITEMDUP.Param -> OR into PetEquip counter.
    if (indices.life_recover_rate != 0 || indices.life_recover_rate_nonzero) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.life_recover_rate, dup_param)) return;
        counters.pet_equip = apply_or(counters.pet_equip, dup_param & pet_equip_dup::PomanRing);
    }
}

void delete_dup_param(DupCounters& counters,
                      const DupParamIndices& indices,
                      const DupParamLookup& lookup,
                      SundrySideEffects& sundry_side_effects) noexcept {
    sundry_side_effects.set_b_street_stall = false;
    sundry_side_effects.clear_b_street_stall = false;

    if (indices.all_plus_value != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.all_plus_value, dup_param)) return;
        // Source's first five flags clear only when set; the later five
        // unconditionally toggle, even when absent before deletion.
        counters.charm = apply_delete(counters.charm, dup_param & 0x3eu);
        counters.charm ^= dup_param & 0xc1c0u;
    }
    if (indices.mugong_num != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.mugong_num, dup_param)) return;
        counters.herb = apply_delete(counters.herb, dup_param & 0x1feu);
    }
    if (indices.mugong_type != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.mugong_type, dup_param)) return;
        counters.incantation = apply_delete(counters.incantation,
            dup_param & (incantation_dup::MemoryMove | incantation_dup::ProtectAll));
    }
    if (indices.life_recover != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.life_recover, dup_param)) return;
        // Legacy: only clear bStreetStall if the StreetStall bit is in BOTH
        // the dup.Param AND the counter. The set flag is captured for
        // orchestrator symmetry.
        const std::uint32_t overlap = counters.sundries & dup_param & sundries_dup::StreetStall;
        counters.sundries ^= overlap;
        if ((dup_param & sundries_dup::StreetStall) &&
            (counters.sundries & sundries_dup::StreetStall) == 0 &&
            (overlap & sundries_dup::StreetStall)) {
            // The StreetStall bit was set in the counter and has just been
            // cleared by the XOR. The orchestrator should write bStreetStall=0.
            sundry_side_effects.clear_b_street_stall = true;
        }
    }
    if (indices.life_recover_rate != 0 || indices.life_recover_rate_nonzero) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.life_recover_rate, dup_param)) return;
        counters.pet_equip = apply_delete(counters.pet_equip, dup_param & pet_equip_dup::PomanRing);
    }
}

bool is_dup_able(const DupCounters& counters,
                 const DupParamIndices& indices,
                 const DupParamLookup& lookup) noexcept {
    if (indices.all_plus_value != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.all_plus_value, dup_param)) return false;
        if (counters.charm & dup_param & 0xc1feu) return false;
    }
    if (indices.mugong_num != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.mugong_num, dup_param)) return false;
        if (counters.herb & dup_param & 0x1feu) return false;
    }
    if (indices.mugong_type != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.mugong_type, dup_param)) return false;
        if (counters.incantation & dup_param & 0x1feu) return false;
    }
    if (indices.life_recover != 0) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.life_recover, dup_param)) return false;
        if (counters.sundries & dup_param & sundries_dup::StreetStall) return false;
    }
    if (indices.life_recover_rate != 0 || indices.life_recover_rate_nonzero) {
        std::uint32_t dup_param = 0;
        if (!lookup.try_get_dup_param(indices.life_recover_rate, dup_param)) return false;
        if (counters.pet_equip & dup_param & pet_equip_dup::PomanRing) return false;
    }
    return true;
}

}  // namespace mxh::server
