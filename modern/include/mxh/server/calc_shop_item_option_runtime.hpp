// calc_shop_item_option_runtime.hpp
//
// Runtime orchestrator for CShopItemManager::CalcShopItemOption. The
// data plane in calc_shop_item_option.hpp returns a per-call decision:
// it mutates ShopItemOption stats in place AND fills out a
// CalcShopItemOptionSideEffects struct with the new ProtectItemIdx
// + 4 expanded-slot flags. This header turns that into side effects
// on a real ShopItemManager (ProtectItemIdx) and dispatches the
// SetExtra*SlotCount calls to a virtual Player hook (production
// wires this to m_pPlayer->SetExtraInvenSlotCount etc.).
//
// 1:1 invariants (1:1 with legacy CShopItemManager::CalcShopItemOption):
//   - m_pPlayer->SetExtraInvenSlotCount(1) when expanded_inven_slot fires.
//   - m_pPlayer->SetExtraPyogukSlotCount(1) when expanded_pyoguk_slot fires.
//   - m_pPlayer->SetExtraMugongSlotCount(1) when expanded_mugong_slot fires.
//   - m_pPlayer->SetExtraCharacterSlot(1) when expanded_character_slot fires.
//   - m_ProtectItemIdx = side_effects.new_protect_item_idx on Ok return.
//
// Pattern mirrors calc_plus_time_runtime.hpp (D4.28): data plane in
// the matching *_data_plane*.hpp, runtime orchestrator also inline
// in this header, tests verify behavior through the public surface.

#pragma once

#include <cstdint>

#include <mxh/game/shop_item_option.hpp>
#include <mxh/server/calc_shop_item_option.hpp>
#include <mxh/server/legacy_shop_item_kind.hpp>
#include <mxh/server/shop_item_manager.hpp>

namespace mxh::server {

// Player-side hook for the SetExtra* slot calls. The legacy code calls
// these directly on CPlayer (m_pPlayer->SetExtraInvenSlotCount etc);
// the modern port uses a virtual interface so the runtime can be
// tested without a real Player instance. Production wires this to a
// thin adapter over the live Player singleton.
class CalcShopItemOptionPlayerHook {
public:
    virtual ~CalcShopItemOptionPlayerHook() = default;
    virtual void on_expand_inven_slot() noexcept = 0;
    virtual void on_expand_pyoguk_slot() noexcept = 0;
    virtual void on_expand_mugong_slot() noexcept = 0;
    virtual void on_expand_character_slot() noexcept = 0;
};

// Outcome: which side effects actually fired during the call. Tests
// use this to assert that the correct player hook was dispatched and
// that the manager's ProtectItemIdx was updated.
struct CalcShopItemOptionRuntimeOutcome {
    bool protect_item_idx_updated = false;
    std::uint32_t protect_item_idx_after = 0;
    bool inven_slot_expanded     = false;
    bool pyoguk_slot_expanded    = false;
    bool mugong_slot_expanded    = false;
    bool character_slot_expanded = false;
};

// Runtime: invokes the data plane once and applies the resulting
// side effects to the manager (ProtectItemIdx) and the player hook
// (SetExtra*SlotCount). Returns the outcome so tests can assert
// which side effects fired.
//
// Production callers pass the live ShopItemManager and a hook bound
// to the live Player; tests pass a fresh manager + a recording hook
// so each test starts with an empty protect_item_idx + a clean
// call recording.
inline CalcShopItemOptionRuntimeOutcome apply_calc_shop_item_option(
    ShopItemManager& mgr,
    mxh::game::ShopItemOption& stats,
    std::uint32_t w_idx,
    bool b_add,
    std::uint32_t param,
    const CalcShopItemOptionInfo& info,
    const CalcShopItemOptionEnv& env,
    CalcShopItemOptionPlayerHook& player_hook) {
    CalcShopItemOptionRuntimeOutcome out;
    CalcShopItemOptionSideEffects side_effects;
    const auto status = calc_shop_item_option(
        stats, w_idx, b_add, param, info, env,
        mgr.protect_item_idx(), side_effects);
    if (status != CalcShopItemOptionStatus::Ok) {
        // Data plane refused (InvalidIcon / ItemInfoMissing / NullStats);
        // no side effects were computed, nothing to apply.
        out.protect_item_idx_after = mgr.protect_item_idx();
        return out;
    }
    // Update ProtectItemIdx on the manager. 1:1 with legacy
    // m_ProtectItemIdx = ... when the CheRyuk incantation branch
    // either sets it (bAdd) or clears it (bAdd==false).
    if (side_effects.new_protect_item_idx != mgr.protect_item_idx()) {
        mgr.set_protect_item_idx(side_effects.new_protect_item_idx);
        out.protect_item_idx_updated = true;
    }
    out.protect_item_idx_after = mgr.protect_item_idx();
    // Dispatch player-side slot expansion hooks. Each one is a 1:1
    // match with the legacy m_pPlayer->SetExtra*SlotCount call.
    if (side_effects.expanded_inven_slot) {
        player_hook.on_expand_inven_slot();
        out.inven_slot_expanded = true;
    }
    if (side_effects.expanded_pyoguk_slot) {
        player_hook.on_expand_pyoguk_slot();
        out.pyoguk_slot_expanded = true;
    }
    if (side_effects.expanded_mugong_slot) {
        player_hook.on_expand_mugong_slot();
        out.mugong_slot_expanded = true;
    }
    if (side_effects.expanded_character_slot) {
        player_hook.on_expand_character_slot();
        out.character_slot_expanded = true;
    }
    return out;
}

}  // namespace mxh::server
