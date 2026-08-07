// legacy_shop_item_kind.hpp
//
// Single source of truth for the legacy eSHOP_ITEM_* enumerator values
// (1:1 with [CC]Header/CommonGameDefine.h:698-713). Other data-plane
// and manager headers include this file instead of redefining the
// constants -- MSVC C2374 forbids initializing the same inline
// constexpr variable twice in one TU, so a single definition is
// mandatory.
//
// Names are unchanged from the legacy header; values match exactly.

#pragma once

#include <cstdint>

namespace mxh::server {

// Legacy eSHOP_ITEM_* values that drive dup-counter dispatch and the
// per-bucket CalcShopItemOption branches.
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_PREMIUM      = 257u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_CHARM         = 258u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_HERB          = 259u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_INCANTATION   = 260u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_MAKEUP        = 261u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_DECORATION    = 262u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_SUNDRIES      = 263u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_EQUIP         = 264u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_PET           = 300u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_PET_EQUIP     = 310u;

}  // namespace mxh::server
