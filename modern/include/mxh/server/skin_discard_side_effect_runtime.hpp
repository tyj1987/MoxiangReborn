// skin_discard_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// skin_discard_side_effect_plan(). The data plane returns an ordered
// list of 3 SkinDiscardSideEffect entries (WriteSkinItemUpdate +
// CharacterSkinInfoUpdate + BroadcastSkinInfo); this header walks the
// list and dispatches each entry to its respective subsystem via
// virtual callback interfaces.
//
// 1:1 invariants (1:1 with legacy CShopItemManager::DiscardSkinItem
// from [Server]Map/ShopItemManager.cpp:2707-2722):
//   3-step chain in legacy order:
//     1. WriteSkinItemUpdate    (legacy RemoveEquipSkin /
//                                m_pPlayer->GetShopItemStats()->wSkinItem)
//     2. CharacterSkinInfoUpdate (legacy CharacterSkinInfoUpdate DB write)
//     3. BroadcastSkinInfo      (legacy SEND_SKIN_INFO broadcast)
//
// Note: unlike PutSkinSelectItem (D4.38), the legacy code does NOT
// reset the skin delay timer on discard -- delay is only set when a
// skin is applied, not when it is removed. The runtime therefore has
// no StartSkinDelay step.
//
// Pattern mirrors put_on_avatar_side_effect_runtime.hpp (D4.36) and
// skin_select_side_effect_runtime.hpp (D4.38): data plane in
// matching header, runtime orchestrator also inline here, tests
// verify behavior through the public surface.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/skin_discard_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the skin-discard side-effect chain.
// Production wires each method to the live subsystem (Player stats
// write, DB thread, PACKEDDATA broadcast). Tests wire them to
// recording stubs so each test starts with empty call lists.
class SkinDiscardSideEffectSink {
public:
    virtual ~SkinDiscardSideEffectSink() = default;

    // Legacy: RemoveEquipSkin / write new wSkinItem[] into
    // m_pPlayer->GetShopItemStats()->wSkinItem. The data plane has
    // already produced the new wSkinItem[]; the orchestrator writes
    // it back into the live Player stats.
    virtual void write_skin_item_update() = 0;

    // Legacy: CharacterSkinInfoUpdate -- DB write of new wSkinItem[].
    virtual void character_skin_info_update() = 0;

    // Legacy: SEND_SKIN_INFO broadcast
    // (Category = MP_ITEMEXT, Protocol = MP_ITEMEXT_SKINITEM_DISCARD_ACK).
    virtual void broadcast_skin_info() = 0;
};

struct SkinDiscardRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t skin_writes     = 0;
    std::size_t db_updates      = 0;
    std::size_t broadcasts      = 0;
    bool broadcast_flag_consumed = false;
};

// Runtime: walks the side-effect plan (3 steps) and dispatches each
// entry in legacy order (write, then db update, then broadcast).
// Returns the outcome counters.
inline SkinDiscardRuntimeOutcome apply_skin_discard_side_effects(
    const SkinDiscardSideEffectPlan& plan,
    SkinDiscardSideEffectSink& sink) {
    SkinDiscardRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case SkinDiscardSideEffectKind::WriteSkinItemUpdate:
            sink.write_skin_item_update();
            ++out.skin_writes;
            ++out.effects_applied;
            break;
        case SkinDiscardSideEffectKind::CharacterSkinInfoUpdate:
            sink.character_skin_info_update();
            ++out.db_updates;
            ++out.effects_applied;
            break;
        case SkinDiscardSideEffectKind::BroadcastSkinInfo:
            sink.broadcast_skin_info();
            ++out.broadcasts;
            ++out.effects_applied;
            break;
        }
    }
    out.broadcast_flag_consumed = plan.send_broadcast;
    return out;
}

}  // namespace mxh::server
