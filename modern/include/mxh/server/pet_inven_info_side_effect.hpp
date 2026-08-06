// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_PETINVEN_INFO_SYN from legacy
// [Server]Map/ItemManager.cpp:4941-4952.
//
// The legacy handler triggers a DB load of pet-inventory item info
// for the player's currently-summoned pet. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. pPlayer->GetPetManager()->GetCurSummonPet() (return if null -
//      no pet summoned, silent drop).
//   3. Fire PetInvenItemOptionInfo (DB query for pet inv items).
//
// The handler does NOT send any ACK/NACK to the client; the data
// arrives later via MP_ITEM broadcasts from the DB callback.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_PETINVEN_INFO_SYN
// (single protocol code, no ACK/NACK pair on the SYN path).
inline constexpr std::uint8_t LEGACY_MP_ITEM_PETINVEN_INFO_SYN = 77u;

// 1:1 with legacy [Server]Map/ItemManager.cpp legacy
// TP_PETINVEN_START/TP_PETINVEN_END. We capture the pair as a
// scalar range so the dispatcher doesn't need to know the absolute
// values.
inline constexpr std::uint16_t LEGACY_TP_PETINVEN_START = 80;
inline constexpr std::uint16_t LEGACY_TP_PETINVEN_END   = 100;

enum class PetInvenInfoOutcome : std::uint8_t {
    Triggered   = 0,  // legacy: player + pet summoned
    NoPlayer    = 1,  // legacy: FindUser returned null
    NoPetActive = 2,  // legacy: GetCurSummonPet() == null
};

struct PetInvenInfoValidationInput final {
    bool player_found = false;
    bool pet_summoned = false;
};

inline PetInvenInfoOutcome classify_pet_inven_info_outcome(
    const PetInvenInfoValidationInput& in) noexcept {
    if (!in.player_found) {
        return PetInvenInfoOutcome::NoPlayer;
    }
    if (!in.pet_summoned) {
        return PetInvenInfoOutcome::NoPetActive;
    }
    return PetInvenInfoOutcome::Triggered;
}

enum class PetInvenInfoSideEffectKind : std::uint8_t {
    FirePetInvenDbQuery = 0,  // legacy PetInvenItemOptionInfo
};

struct PetInvenInfoSideEffect final {
    PetInvenInfoSideEffectKind kind =
        PetInvenInfoSideEffectKind::FirePetInvenDbQuery;
    std::uint32_t object_id = 0;     // legacy pPlayer->GetID()
    std::uint32_t user_id = 0;       // legacy pPlayer->GetUserID()
    std::uint16_t start_pos = 0;     // legacy TP_PETINVEN_START
    std::uint16_t end_pos = 0;       // legacy TP_PETINVEN_END
};

struct PetInvenInfoSideEffectPlan final {
    std::vector<PetInvenInfoSideEffect> effects;
    bool trigger_db = false;
};

inline PetInvenInfoSideEffectPlan pet_inven_info_side_effect_plan(
    const PetInvenInfoValidationInput& in,
    std::uint32_t object_id,
    std::uint32_t user_id) {
    PetInvenInfoSideEffectPlan plan;
    const PetInvenInfoOutcome outcome =
        classify_pet_inven_info_outcome(in);
    if (outcome != PetInvenInfoOutcome::Triggered) {
        return plan;
    }
    plan.trigger_db = true;
    plan.effects.reserve(1u);
    PetInvenInfoSideEffect db{};
    db.kind = PetInvenInfoSideEffectKind::FirePetInvenDbQuery;
    db.object_id = object_id;
    db.user_id = user_id;
    db.start_pos = LEGACY_TP_PETINVEN_START;
    db.end_pos = LEGACY_TP_PETINVEN_END;
    plan.effects.push_back(db);
    return plan;
}

}  // namespace mxh::server
