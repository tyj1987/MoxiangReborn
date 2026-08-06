// extra_slot_count.hpp
//
// 1:1 port of legacy CInventorySlot::SetExtraSlotCount(DWORD Count) /
// CPyogukSlot::SetExtraSlotCount(DWORD Count) from
// [Server]Map/InventorySlot.cpp + PyogukSlot.cpp.

// The legacy implementation is wrapped in #ifdef _JAPAN_LOCAL_ /
// _HK_LOCAL_ / _TL_LOCAL_ and computes:
//   m_ExtraSlotCount = Count;
//   m_SlotNum        = TABCELL_XXX_NUM * (GIVEN_XXX_SLOT + m_ExtraSlotCount);

// Pure data plane: takes the per-cell count + given-slot count +
// the extra count and returns the new m_SlotNum. The m_ExtraSlotCount
// mutation is trivial (just stores the count). The data plane
// factors the cell-count arithmetic into a free function so
// the orchestrator (legacy InventorySlot) can apply the same
// arithmetic without dragging in the locale macros.

// 1:1 invariants:
//   - cells_per_tab * (given + extra) with integer overflow at
//     32 bits (legacy uses DWORD which is 32-bit on legacy).
//     Modern port uses std::uint32_t.
//   - Locale splits: CN/KR does NOT extend slots (SetExtraSlotCount
//     is a no-op there). The legacy code gates the function on
//     _JAPAN_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_; modern port
//     delegates the locale decision to the caller.

#pragma once

#include <cstdint>

namespace mxh::server {

// 1:1 with [CC]Header/CommonGameDefine.h cell counts.
inline constexpr std::uint32_t LEGACY_TABCELL_INVENTORY_NUM = 20u;
inline constexpr std::uint32_t LEGACY_TABCELL_PYOGUK_NUM   = 30u;

// 1:1 with [CC]Header/CommonGameDefine.h given-slot counts.
// The legacy #define values differ by locale (JP pyoguk = 3, others = 2);
// the modern port uses the locale-agnostic value 2 and documents
// the JP override as a separate constant for callers that need it.
inline constexpr std::uint32_t LEGACY_GIVEN_INVENTORY_SLOT_DEFAULT = 2u;
inline constexpr std::uint32_t LEGACY_GIVEN_PYOGUK_SLOT_DEFAULT   = 2u;
inline constexpr std::uint32_t LEGACY_GIVEN_PYOGUK_SLOT_JP       = 3u;

// 1:1 with legacy CInventorySlot::SetExtraSlotCount cell-count
// arithmetic: m_SlotNum = cells_per_tab * (given + extra_count).
// The caller picks cells_per_tab and given based on the slot
// type (inventory vs pyoguk) and locale (CN/KR vs JP/HK/TL).
inline std::uint32_t compute_extra_slot_total(
    std::uint32_t cells_per_tab,
    std::uint32_t given_slots,
    std::uint32_t extra_count) noexcept {
    return cells_per_tab * (given_slots + extra_count);
}

// Convenience wrappers for the two legacy call sites.
// 1:1 with CInventorySlot::SetExtraSlotCount (inventory cell count 20,
// given = 2; CN/KR returns base 40, +20 per extra).
inline std::uint32_t compute_inventory_slot_total(
    std::uint32_t extra_count) noexcept {
    return compute_extra_slot_total(
        LEGACY_TABCELL_INVENTORY_NUM,
        LEGACY_GIVEN_INVENTORY_SLOT_DEFAULT,
        extra_count);
}

// 1:1 with CPyogukSlot::SetExtraSlotCount (pyoguk cell count 30,
// given = 2 by default; JP override = 3).
inline std::uint32_t compute_pyoguk_slot_total_default(
    std::uint32_t extra_count) noexcept {
    return compute_extra_slot_total(
        LEGACY_TABCELL_PYOGUK_NUM,
        LEGACY_GIVEN_PYOGUK_SLOT_DEFAULT,
        extra_count);
}

inline std::uint32_t compute_pyoguk_slot_total_jp(
    std::uint32_t extra_count) noexcept {
    return compute_extra_slot_total(
        LEGACY_TABCELL_PYOGUK_NUM,
        LEGACY_GIVEN_PYOGUK_SLOT_JP,
        extra_count);
}

}  // namespace mxh::server