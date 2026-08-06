//
// CItemManager::MP_ITEMEXT_UNIQUEITEM_MIX_SYN from legacy
// [Server]Map/ItemManager.cpp:6397-6521.
//
// The legacy handler runs 4 gates and either aborts silently (no
// NACK message sent) or performs the full mix:
//   1. CHKRT->ItemOf(pPlayer, wBasicItemPos, wBasicItemIdx, 0, 0,
//      CB_EXIST|CB_ICONIDX) - basic item exists.
//   2. For each material: CHKRT->ItemOf(pPlayer, Material.ItemPos,
//      Material.wItemIdx, Material.Dur, 0, CB_EXIST|CB_ICONIDX), and
//      Material.ItemPos != wBasicItemPos.
//   3. pInfo = GAMERESRCMNGR->GetUniqueItemMixList(wBasicItemIdx)
//      exists.
//   4. For each material kind in pInfo: EnoughMixMaterial returns
//      true.
//
// On success:
//   a. Discard each material (DiscardItem per material); send
//      MSG_ITEM_DISCARD_ACK {MP_ITEMEXT, DELETEITEM, TargetPos,
//      wItemIdx, ItemNum} per discard.
//   b. LogItemMoney (eLog_ItemDiscard) per discard.
//   c. Discard basic item; send MSG_ITEM_DISCARD_ACK {MP_ITEMEXT,
//      DELETEITEM, basicPos, basicIdx, 1}.
//   d. LogItemMoney (eLog_ItemDiscard).
//   e. Roll random seed (1..100), pick result item index from
//      pInfo->sUniqueItemMixResult[5] weighted by wResultRate.
//   f. obtainItemNum = GetCanBuyNumInSpace(...); if 0, return
//      (silent).
//   g. ObtainItemEx(pPlayer, ..., MP_ITEMEXT_UNIQUEITEM_MIX_ACK, ...)
//      which sends ACK.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

inline constexpr std::uint32_t LEGACY_UNIQUE_MIX_RANDOM_MIN  = 1u;
inline constexpr std::uint32_t LEGACY_UNIQUE_MIX_RANDOM_MAX  = 100u;
inline constexpr std::uint32_t LEGACY_UNIQUE_MIX_RESULT_SLOTS = 5u;

enum class UniqueItemMixOutcome : std::uint8_t {
    Mixed             = 0,
    NoSpaceForResult  = 1,  // legacy: success path but no inventory room
    BasicItemMissing  = 2,  // legacy gate 1
    MaterialMissing   = 3,  // legacy gate 2
    MixInfoMissing    = 4,  // legacy gate 3
    NotEnoughMaterial = 5,  // legacy gate 4
};

struct UniqueItemMixValidationInput final {
    bool basic_item_exists = false;
    bool all_materials_exist = false;          // gate 2
    bool mix_info_exists = false;              // gate 3
    bool enough_material_for_each_kind = false; // gate 4
    bool inventory_has_space = false;          // obtainItemNum != 0
};

inline UniqueItemMixOutcome classify_unique_item_mix_outcome(
    const UniqueItemMixValidationInput& in) noexcept {
    if (!in.basic_item_exists) {
        return UniqueItemMixOutcome::BasicItemMissing;
    }
    if (!in.all_materials_exist) {
        return UniqueItemMixOutcome::MaterialMissing;
    }
    if (!in.mix_info_exists) {
        return UniqueItemMixOutcome::MixInfoMissing;
    }
    if (!in.enough_material_for_each_kind) {
        return UniqueItemMixOutcome::NotEnoughMaterial;
    }
    if (!in.inventory_has_space) {
        return UniqueItemMixOutcome::NoSpaceForResult;
    }
    return UniqueItemMixOutcome::Mixed;
}

enum class UniqueItemMixSideEffectKind : std::uint8_t {
    DiscardMaterialItem     = 0,
    SendMaterialDeleteAck   = 1,  // MSG_ITEM_DISCARD_ACK DELETEITEM
    LogMaterialDiscard      = 2,
    DiscardBasicItem        = 3,
    SendBasicDeleteAck      = 4,
    LogBasicDiscard         = 5,
    RollRandomResultItem    = 6,
    ObtainResultItem        = 7,  // legacy ObtainItemEx -> sends ACK
};

struct UniqueItemMixMaterial final {
    std::uint16_t pos = 0;
    std::uint16_t w_icon_idx = 0;
    std::uint32_t db_idx = 0;
    std::uint16_t dur = 0;
};

struct UniqueItemMixSideEffect final {
    UniqueItemMixSideEffectKind kind =
        UniqueItemMixSideEffectKind::DiscardMaterialItem;
    std::uint32_t player_id = 0;
    UniqueItemMixMaterial material{};
    std::uint32_t basic_db_idx = 0;
    std::uint32_t basic_pos = 0;
    std::uint16_t basic_w_idx = 0;
    std::uint32_t result_w_idx = 0;  // rolled result
    std::uint16_t obtain_num = 0;
};

struct UniqueItemMixSideEffectPlan final {
    std::vector<UniqueItemMixSideEffect> effects;
    bool discard_materials = false;
    bool discard_basic = false;
    bool roll_result = false;
    bool obtain_result = false;
    bool any_log = false;
};

inline UniqueItemMixSideEffectPlan unique_item_mix_side_effect_plan(
    const UniqueItemMixValidationInput& in,
    std::uint32_t player_id,
    const std::vector<UniqueItemMixMaterial>& materials,
    std::uint16_t basic_pos,
    std::uint16_t basic_w_idx,
    std::uint32_t basic_db_idx,
    std::uint32_t result_w_idx,
    std::uint16_t obtain_num) {
    UniqueItemMixSideEffectPlan plan;
    const UniqueItemMixOutcome outcome =
        classify_unique_item_mix_outcome(in);

    if (outcome == UniqueItemMixOutcome::Mixed) {
        plan.discard_materials = true;
        plan.discard_basic = true;
        plan.roll_result = true;
        plan.obtain_result = true;
        plan.any_log = true;
        plan.effects.reserve(2u * materials.size() + 4u);

        for (const auto& m : materials) {
            UniqueItemMixSideEffect disc{};
            disc.kind = UniqueItemMixSideEffectKind::DiscardMaterialItem;
            disc.player_id = player_id;
            disc.material = m;
            plan.effects.push_back(disc);
            UniqueItemMixSideEffect ack{};
            ack.kind = UniqueItemMixSideEffectKind::SendMaterialDeleteAck;
            ack.player_id = player_id;
            ack.material = m;
            plan.effects.push_back(ack);
            UniqueItemMixSideEffect log{};
            log.kind = UniqueItemMixSideEffectKind::LogMaterialDiscard;
            log.player_id = player_id;
            log.material = m;
            plan.effects.push_back(log);
        }

        UniqueItemMixSideEffect basic_disc{};
        basic_disc.kind = UniqueItemMixSideEffectKind::DiscardBasicItem;
        basic_disc.player_id = player_id;
        basic_disc.basic_pos = basic_pos;
        basic_disc.basic_w_idx = basic_w_idx;
        basic_disc.basic_db_idx = basic_db_idx;
        plan.effects.push_back(basic_disc);
        UniqueItemMixSideEffect basic_ack{};
        basic_ack.kind = UniqueItemMixSideEffectKind::SendBasicDeleteAck;
        basic_ack.player_id = player_id;
        basic_ack.basic_pos = basic_pos;
        basic_ack.basic_w_idx = basic_w_idx;
        plan.effects.push_back(basic_ack);
        UniqueItemMixSideEffect basic_log{};
        basic_log.kind = UniqueItemMixSideEffectKind::LogBasicDiscard;
        basic_log.player_id = player_id;
        basic_log.basic_pos = basic_pos;
        basic_log.basic_w_idx = basic_w_idx;
        basic_log.basic_db_idx = basic_db_idx;
        plan.effects.push_back(basic_log);
        UniqueItemMixSideEffect roll{};
        roll.kind = UniqueItemMixSideEffectKind::RollRandomResultItem;
        roll.player_id = player_id;
        roll.result_w_idx = result_w_idx;
        plan.effects.push_back(roll);
        UniqueItemMixSideEffect obtain{};
        obtain.kind = UniqueItemMixSideEffectKind::ObtainResultItem;
        obtain.player_id = player_id;
        obtain.result_w_idx = result_w_idx;
        obtain.obtain_num = obtain_num;
        plan.effects.push_back(obtain);
        return plan;
    }

    if (outcome == UniqueItemMixOutcome::NoSpaceForResult) {
        plan.discard_materials = true;
        plan.discard_basic = true;
        plan.roll_result = true;
        plan.obtain_result = false;
        plan.any_log = true;
        plan.effects.reserve(2u * materials.size() + 4u);
        for (const auto& m : materials) {
            UniqueItemMixSideEffect disc{};
            disc.kind = UniqueItemMixSideEffectKind::DiscardMaterialItem;
            disc.player_id = player_id;
            disc.material = m;
            plan.effects.push_back(disc);
            UniqueItemMixSideEffect ack{};
            ack.kind = UniqueItemMixSideEffectKind::SendMaterialDeleteAck;
            ack.player_id = player_id;
            ack.material = m;
            plan.effects.push_back(ack);
            UniqueItemMixSideEffect log{};
            log.kind = UniqueItemMixSideEffectKind::LogMaterialDiscard;
            log.player_id = player_id;
            log.material = m;
            plan.effects.push_back(log);
        }
        UniqueItemMixSideEffect basic_disc{};
        basic_disc.kind = UniqueItemMixSideEffectKind::DiscardBasicItem;
        basic_disc.player_id = player_id;
        basic_disc.basic_pos = basic_pos;
        basic_disc.basic_w_idx = basic_w_idx;
        basic_disc.basic_db_idx = basic_db_idx;
        plan.effects.push_back(basic_disc);
        UniqueItemMixSideEffect basic_ack{};
        basic_ack.kind = UniqueItemMixSideEffectKind::SendBasicDeleteAck;
        basic_ack.player_id = player_id;
        basic_ack.basic_pos = basic_pos;
        basic_ack.basic_w_idx = basic_w_idx;
        plan.effects.push_back(basic_ack);
        UniqueItemMixSideEffect basic_log{};
        basic_log.kind = UniqueItemMixSideEffectKind::LogBasicDiscard;
        basic_log.player_id = player_id;
        basic_log.basic_pos = basic_pos;
        basic_log.basic_w_idx = basic_w_idx;
        basic_log.basic_db_idx = basic_db_idx;
        plan.effects.push_back(basic_log);
        UniqueItemMixSideEffect roll{};
        roll.kind = UniqueItemMixSideEffectKind::RollRandomResultItem;
        roll.player_id = player_id;
        roll.result_w_idx = result_w_idx;
        plan.effects.push_back(roll);
        return plan;
    }

    // All other failure paths: silent (legacy uses bare 'break', no
    // NACK message).
    return plan;
}

inline std::uint32_t unique_item_mix_random_seed(std::uint32_t seed) noexcept {
    if (seed < LEGACY_UNIQUE_MIX_RANDOM_MIN) {
        return LEGACY_UNIQUE_MIX_RANDOM_MIN;
    }
    if (seed > LEGACY_UNIQUE_MIX_RANDOM_MAX) {
        return LEGACY_UNIQUE_MIX_RANDOM_MAX;
    }
    return seed;
}

}  // namespace mxh::server
