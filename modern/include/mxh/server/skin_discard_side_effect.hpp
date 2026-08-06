// 1:1 side-effect-dispatcher port of CShopItemManager::DiscardSkinItem
// from legacy [Server]Map/ShopItemManager.cpp:2707-2722.
//
// After DiscardSkinItem (or the per-row ResetSkinTable walk in
// RemoveEquipSkin) succeeds, the legacy code applies:
//   1. RemoveEquipSkin(dwItemIndex->ItemKind) (legacy: clears
//      wSkinItem[] in place by walking the matching skin table).
//   2. CharacterSkinInfoUpdate (DB write of the new wSkinItem[]).
//   3. SEND_SKIN_INFO broadcast (Category = MP_ITEMEXT,
//      Protocol = MP_ITEMEXT_SKINITEM_DISCARD_ACK).
//
// Unlike PutSkinSelectItem, the legacy code does NOT reset the skin
// delay timer on discard (delay is only set when a skin is applied,
// not when it is removed). The data plane reflects that distinction.
//
// The data plane below captures those effects in a structured payload
// so the orchestrator can route them to the runtime player + DBThread
// + PACKEDDATA_OBJ subsystems without re-reading the legacy body.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <mxh/server/skin_discard_transition.hpp>

namespace mxh::server {

// 1:1 with legacy MP_ITEMEXT_SKINITEM_DISCARD_ACK protocol code.
// (Category = MP_ITEMEXT, Protocol = MP_ITEMEXT_SKINITEM_DISCARD_ACK.)
enum class SkinDiscardSideEffectKind : std::uint8_t {
    WriteSkinItemUpdate = 0,      // legacy RemoveEquipSkin (the data plane
                                  // already produced the new wSkinItem[];
                                  // the orchestrator writes it back into
                                  // m_pPlayer->GetShopItemStats()->wSkinItem)
    CharacterSkinInfoUpdate = 1,  // legacy CharacterSkinInfoUpdate() DB write
    BroadcastSkinInfo = 2,        // legacy SEND_SKIN_INFO broadcast
};

struct SkinDiscardSideEffect final {
    SkinDiscardSideEffectKind kind =
        SkinDiscardSideEffectKind::WriteSkinItemUpdate;
};

struct SkinDiscardSideEffectPlan final {
    std::vector<SkinDiscardSideEffect> effects;
    bool send_broadcast = false;
};

// 1:1 with legacy CShopItemManager::DiscardSkinItem success path.
//
// The legacy function unconditionally applies the 3-step chain (no
// failure case). The data plane mirrors that: the plan always has
// 3 steps and send_broadcast is always true.
inline SkinDiscardSideEffectPlan skin_discard_side_effect_plan() {
    SkinDiscardSideEffectPlan plan;
    plan.effects.reserve(3u);
    plan.send_broadcast = true;

    SkinDiscardSideEffect write{};
    write.kind = SkinDiscardSideEffectKind::WriteSkinItemUpdate;
    plan.effects.push_back(write);

    SkinDiscardSideEffect db{};
    db.kind = SkinDiscardSideEffectKind::CharacterSkinInfoUpdate;
    plan.effects.push_back(db);

    SkinDiscardSideEffect broadcast{};
    broadcast.kind = SkinDiscardSideEffectKind::BroadcastSkinInfo;
    plan.effects.push_back(broadcast);

    return plan;
}

}  // namespace mxh::server
